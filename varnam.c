/* varnam.c

Copyright (C) 2010 Navaneeth.K.N

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>
#include <stdarg.h>

#include "varnam-api.h"
#include "varnam-types.h"
#include "varnam-util.h"
#include "varnam-result-codes.h"
#include "varnam-symbol-table.h"
#include "rendering/renderers.h"

static struct varnam_token_rendering renderers[] = {
    { "ml-unicode", ml_unicode_renderer, ml_unicode_rtl_renderer }
};

static struct varnam_internal*
initialize_internal()
{
    struct varnam_internal *vi;
    vi = (struct varnam_internal *) xmalloc(sizeof (struct varnam_internal));
    if(vi) {
        vi->virama[0] = '\0';
        vi->last_token_available = 0;
        vi->last_rtl_token_available = 0;
        vi->last_token = NULL;
        vi->last_rtl_token = NULL;
        vi->current_token = NULL;
        vi->current_rtl_token = NULL;
        vi->output = strbuf_init(100);
        vi->rtl_output = strbuf_init(100);
        vi->last_error = strbuf_init(100);
        vi->lookup = strbuf_init(10);
        vi->log_level = VARNAM_LOG_DEFAULT;
        vi->log_callback = NULL;
        vi->log_message = strbuf_init(100);
        vi->vst_buffering = 0;

        /* scheme details buffers */
        vi->scheme_language_code = strbuf_init(2);
        vi->scheme_identifier = strbuf_init(10);
        vi->scheme_display_name = strbuf_init(10);
        vi->scheme_author = strbuf_init(25);
        vi->scheme_compiled_date = strbuf_init(10);

        /* configuration options */
        vi->config_use_dead_consonants = 1;
        vi->config_ignore_duplicate_tokens = 0;
    }
    return vi;
}

int
varnam_init(const char *scheme_file, varnam **handle, char **msg)
{
    int rc;
    varnam *c;
    struct varnam_internal *vi;
    size_t filename_length;

    if(scheme_file == NULL)
        return VARNAM_ARGS_ERROR;

    c = (varnam *) xmalloc(sizeof (varnam));
    if(!c)
        return VARNAM_MEMORY_ERROR;

    vi = initialize_internal();
    if(!vi)
        return VARNAM_MEMORY_ERROR;

    vi->message = (char *) xmalloc(sizeof (char) * VARNAM_LIB_TEMP_BUFFER_SIZE);
    if(!vi->message)
        return VARNAM_MEMORY_ERROR;

    rc = sqlite3_open(scheme_file, &vi->db);
    if( rc ) {
        asprintf(msg, "Can't open %s: %s\n", scheme_file, sqlite3_errmsg(vi->db));
        sqlite3_close(vi->db);
        return VARNAM_STORAGE_ERROR;
    }

    filename_length = strlen(scheme_file);
    c->scheme_file = (char *) xmalloc(filename_length + 1);
    if(!c->scheme_file)
        return VARNAM_MEMORY_ERROR;

    strncpy(c->scheme_file, scheme_file, filename_length + 1);
    vi->renderers = renderers;
    c->internal = vi;

    rc = ensure_schema_exist(c, msg);
    if (rc != VARNAM_SUCCESS)
        return rc;

    *handle = c;
    return VARNAM_SUCCESS;
}

int
varnam_set_scheme_details(
    varnam *handle,
    const char *language_code,
    const char *identifier,
    const char *display_name,
    const char *author,
    const char *compiled_date)
{
    int rc;

    set_last_error (handle, NULL);

    if (language_code != NULL && strlen(language_code) > 0)
    {
        if (strlen(language_code) != 2)
        {
            set_last_error (handle, "Language code should be one of ISO 639-1 two letter codes.");
            return VARNAM_ERROR;
        }

        rc = vst_add_metadata (handle, VARNAM_METADATA_SCHEME_LANGUAGE_CODE, language_code);
        if (rc != VARNAM_SUCCESS)
            return rc;

        varnam_log (handle, "Set language code to : %s", language_code);
    }

    if (identifier != NULL && strlen(identifier) > 0)
    {
        rc = vst_add_metadata (handle, VARNAM_METADATA_SCHEME_IDENTIFIER, identifier);
        if (rc != VARNAM_SUCCESS)
            return rc;

        varnam_log (handle, "Set language identifier to : %s", identifier);
    }

    if (display_name != NULL && strlen(display_name) > 0)
    {
        rc = vst_add_metadata (handle, VARNAM_METADATA_SCHEME_DISPLAY_NAME, display_name);
        if (rc != VARNAM_SUCCESS)
            return rc;

        varnam_log (handle, "Set language display name to : %s", display_name);
    }

    if (author != NULL && strlen(author) > 0)
    {
        rc = vst_add_metadata (handle, VARNAM_METADATA_SCHEME_AUTHOR, author);
        if (rc != VARNAM_SUCCESS)
            return rc;

        varnam_log (handle, "Set author to : %s", author);
    }

    if (compiled_date != NULL && strlen(compiled_date) > 0)
    {
        rc = vst_add_metadata (handle, VARNAM_METADATA_SCHEME_COMPILED_DATE, compiled_date);
        if (rc != VARNAM_SUCCESS)
            return rc;

        varnam_log (handle, "Set compiled date to : %s", compiled_date);
    }

    return VARNAM_SUCCESS;
}

static const char*
get_scheme_details(varnam *handle, const char* key, struct strbuf *buffer)
{
    struct strbuf *value = buffer;

    if (handle == NULL)
        return NULL;

    if (strbuf_is_blank (value))
    {
        vst_get_metadata (handle, key, value);
    }

    return strbuf_to_s (value);
}

const char*
varnam_get_scheme_language_code(varnam *handle)
{
    return get_scheme_details (handle,
                               VARNAM_METADATA_SCHEME_LANGUAGE_CODE,
                               handle->internal->scheme_language_code);
}

const char*
varnam_get_scheme_identifier(varnam *handle)
{
    return get_scheme_details (handle,
                               VARNAM_METADATA_SCHEME_IDENTIFIER,
                               handle->internal->scheme_identifier);
}

const char*
varnam_get_scheme_display_name(varnam *handle)
{
    return get_scheme_details (handle,
                               VARNAM_METADATA_SCHEME_DISPLAY_NAME,
                               handle->internal->scheme_display_name);
}

const char*
varnam_get_scheme_author(varnam *handle)
{
    return get_scheme_details (handle,
                               VARNAM_METADATA_SCHEME_AUTHOR,
                               handle->internal->scheme_author);
}

const char*
varnam_get_scheme_compiled_date(varnam *handle)
{
    return get_scheme_details (handle,
                               VARNAM_METADATA_SCHEME_COMPILED_DATE,
                               handle->internal->scheme_compiled_date);
}

const char*
varnam_get_last_error(varnam *handle)
{
    if (handle == NULL)
        return NULL;

    return handle->internal->last_error->buffer;
}

int
varnam_enable_logging(varnam *handle, int log_type, void (*callback)(const char*))
{
    if (handle == NULL)
        return VARNAM_ARGS_ERROR;

    if (log_type != VARNAM_LOG_DEFAULT && log_type != VARNAM_LOG_DEBUG)
    {
        set_last_error (handle, "Incorrect log type. Valid values are VARNAM_LOG_DEFAULT or VARNAM_LOG_DEBUG");
        return VARNAM_ERROR;
    }

    handle->internal->log_level = log_type;
    handle->internal->log_callback = callback;

    return VARNAM_SUCCESS;
}

/* Checks if the string has inherent 'a' sound. If yes, we can infer dead consonant from it */
static int can_generate_dead_consonant(const char *string, size_t len)
{
    if(len <= 1)
        return 0;
    return string[len - 2] != 'a' && string[len - 1] == 'a';
}

int
varnam_create_token(
    varnam *handle,
    const char *pattern,
    const char *value1,
    const char *value2,
    int token_type,
    int match_type,
    int buffered)
{
    int rc;
    size_t pattern_len;
    char p[VARNAM_SYMBOL_MAX], v1[VARNAM_SYMBOL_MAX], v2[VARNAM_SYMBOL_MAX];
    struct token *virama;

    set_last_error (handle, NULL);

    if (handle == NULL || pattern == NULL || value1 == NULL)
        return VARNAM_ARGS_ERROR;

    if (strlen(pattern) > VARNAM_SYMBOL_MAX ||
        strlen(value1) > VARNAM_SYMBOL_MAX  ||
        (value2 != NULL && strlen(value2) > VARNAM_SYMBOL_MAX))
    {
        set_last_error (handle, "Length of pattern, value1 or value2 should be less than VARNAM_SYMBOL_MAX");
        return VARNAM_ARGS_ERROR;
    }

    if (match_type != VARNAM_MATCH_EXACT && match_type != VARNAM_MATCH_POSSIBILITY)
    {
        set_last_error (handle, "match_type should be either VARNAM_MATCH_EXACT or VARNAM_MATCH_POSSIBILITY");
        return VARNAM_ARGS_ERROR;
    }

    if (buffered)
    {
        rc = vst_start_buffering (handle);
        if (rc != VARNAM_SUCCESS)
            return rc;
    }

    pattern_len = strlen(pattern);

    if (token_type == VARNAM_TOKEN_CONSONANT &&
        handle->internal->config_use_dead_consonants)
    {
        rc = vst_get_virama(handle, &virama);
        if (rc != VARNAM_SUCCESS)
            return rc;
        else if (virama == NULL)
        {
            set_last_error (handle, "Virama needs to be set before auto generating dead consonants");
            return VARNAM_ERROR;
        }

        if (utf8_ends_with(value1, virama->value1))
        {
            token_type = VARNAM_TOKEN_DEAD_CONSONANT;
        }
        else if (can_generate_dead_consonant(pattern, pattern_len))
        {
            substr(p, pattern, 1, (int) (pattern_len - 1));
            snprintf(v1, VARNAM_SYMBOL_MAX, "%s%s", value1, virama->value1);

            if (value2 != NULL)
                snprintf(v2, VARNAM_SYMBOL_MAX, "%s%s", value2, virama->value1);
            else
                v2[0] = '\0';

            rc = vst_persist_token (handle, p, v1, v2, VARNAM_TOKEN_DEAD_CONSONANT, match_type);
            if (rc != VARNAM_SUCCESS)
            {
                if (buffered) vst_discard_changes(handle);
                return rc;
            }

        }
    }

    rc = vst_persist_token (handle, pattern, value1, value2, token_type, match_type);
    if (rc != VARNAM_SUCCESS)
    {
        if (buffered) vst_discard_changes(handle);
    }

    return rc;
}

int
varnam_get_all_tokens(
    varnam *handle,
    int token_type,
    struct token **tokens)
{
    if (handle == NULL)
        return VARNAM_ARGS_ERROR;

    return vst_get_all_tokens (handle, token_type, tokens);
}

int
varnam_generate_cv_combinations(varnam* handle)
{
    if (handle == NULL)
        return VARNAM_ARGS_ERROR;

    set_last_error (handle, NULL);

    return vst_generate_cv_combinations(handle);
}

int
varnam_flush_buffer(varnam *handle)
{
    if (handle == NULL)
        return VARNAM_ARGS_ERROR;

    return vst_flush_changes(handle);
}

int
varnam_config(varnam *handle, int type, ...)
{
    va_list args;
    int rc = VARNAM_SUCCESS;

    if (handle == NULL)
        return VARNAM_ARGS_ERROR;

    set_last_error (handle, NULL);

    va_start (args, type);
    switch (type)
    {
    case VARNAM_CONFIG_USE_DEAD_CONSONANTS:
        handle->internal->config_use_dead_consonants = va_arg(args, int);
        break;
    case VARNAM_CONFIG_IGNORE_DUPLICATE_TOKEN:
        handle->internal->config_ignore_duplicate_tokens = va_arg(args, int);
        break;
    default:
        set_last_error (handle, "Invalid configuration key");
        rc = VARNAM_INVALID_CONFIG;
    }

    va_end (args);

    return rc;
}

int
varnam_destroy(varnam *handle)
{
    struct varnam_internal *vi;
    int rc;

    if (handle == NULL)
        return VARNAM_ARGS_ERROR;

    vi = handle->internal;

    xfree(vi->message);
    strbuf_destroy (vi->output);
    strbuf_destroy (vi->rtl_output);
    strbuf_destroy (vi->lookup);
    strbuf_destroy (vi->last_error);

    strbuf_destroy (vi->scheme_language_code);
    strbuf_destroy (vi->scheme_identifier);
    strbuf_destroy (vi->scheme_display_name);
    strbuf_destroy (vi->scheme_author);
    strbuf_destroy (vi->scheme_compiled_date);

    xfree(vi->last_token);
    xfree(vi->current_token);
    xfree(vi->last_rtl_token);
    xfree(vi->current_rtl_token);
    rc = sqlite3_close(handle->internal->db);
    if (rc != SQLITE_OK) {
        return VARNAM_ERROR;
    }
    xfree(handle->internal);
    xfree(handle->scheme_file);
    xfree(handle);
    return VARNAM_SUCCESS;
}


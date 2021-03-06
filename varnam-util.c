/* util.c
 *
 * Copyright (C) 2010 Navaneeth.K.N
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "varnam-util.h"
#include "varnam-types.h"

/**
* substr(str,start,length,output) writes length characters of str beginning with start to substring.
* start is is 1-indexed and string should be valid UTF8.
**/
void
substr(char *substring, const char *string, int start, int len)
{
    unsigned int bytes, i;
    const unsigned char *str2, *input;
    unsigned char *sub;

    if(start <= 0) return;
    if(len <= 0) return;

    input = (const unsigned char*) string;
    sub = (unsigned char*) substring;
    --start;

    while( *input && start ) {
        SKIP_MULTI_BYTE_SEQUENCE(input);
        --start;
    }

    for(str2 = input; *str2 && len; len--) {
        SKIP_MULTI_BYTE_SEQUENCE(str2);
    }

    bytes = (unsigned int) (str2 - input);
    for(i = 0; i < bytes; i++) {
        *sub++ = *input++;
    }
    *sub = '\0';
}

/**
 * calculates length of the UTF8 encoded string.
 * length will be the total number of characters and not the bytes
 **/
int
utf8_length(const char *string)
{
    const unsigned char *ustring;
    int len;

    len = 0;
    ustring = (const unsigned char*) string;

    while( *ustring ) {
        ++len;
        SKIP_MULTI_BYTE_SEQUENCE(ustring);
    }

    return len;
}

int
utf8_ends_with(const char *buffer, const char *tocheck)
{
    size_t to_check_len, buffer_len, newlen;
    const char *buf;

    if(!tocheck) return 0;

    to_check_len = strlen(tocheck);
    buffer_len = strlen(buffer);

    if(buffer_len < to_check_len) return 0;

    newlen = (buffer_len - to_check_len);

    buf = buffer + newlen;

    return (strcmp(buf, tocheck) == 0);
}

/* return true if string1 starts with string2 */
int
startswith(const char *string1, const char *string2)
{
    for(; ; string1++, string2++) {
        if(!*string2) {
            break;
        }
        else if(*string1 != *string2) {
            return 0;
        }
    }
    return 1;
}

void*
xmalloc(size_t size)
{
    void *ret = malloc(size);
    return ret;
}

void
xfree (void *ptr)
{
    if(ptr)
        free(ptr);
}

void
set_last_error(varnam *handle, const char *format, ...)
{
    struct strbuf *last_error;
    va_list args;

    last_error = handle->internal->last_error;

    strbuf_clear (last_error);
    if (format != NULL)
    {
        va_start (args, format);
        strbuf_addvf (last_error, format, args);
        va_end (args);
    }
}

void varnam_debug_log(varnam* handle, const char *format, ...)
{
    va_list args;
    struct strbuf *log_message = handle->internal->log_message;

    if (handle->internal->log_level == VARNAM_LOG_DEBUG && handle->internal->log_callback != NULL)
    {
        strbuf_clear (log_message);

        va_start (args, format);
        strbuf_addvf(log_message, format, args);
        va_end(args);

        handle->internal->log_callback(log_message->buffer);
    }
}

void varnam_log(varnam* handle, const char *format, ...)
{
    va_list args;
    struct strbuf *log_message = handle->internal->log_message;

    if (handle->internal->log_callback != NULL)
    {
        strbuf_clear (log_message);

        va_start (args, format);
        strbuf_addvf(log_message, format, args);
        va_end(args);

        handle->internal->log_callback(log_message->buffer);
    }
}


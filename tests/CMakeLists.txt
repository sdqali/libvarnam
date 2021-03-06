
cmake_minimum_required (VERSION 2.6)

project (tests)
message ("Generating project ${PROJECT_NAME}")

file(REMOVE_RECURSE test_output)
file(MAKE_DIRECTORY test_output)

# Append the files that has tests here
list (APPEND TEST_FILES
  00-init.c
  01-transliteration.c  
  02-ml-unicode.c
  03-vst-compilation.c
  )

set(test_executable_name runner)

add_executable(runner test-runner.c ${TEST_FILES})

target_link_libraries(runner ${VARNAM_LIBRARY_NAME})

# This is not a test, but does the cleanup job. I haven't found a way to add pre/post hooks to tests.
# this should be replaced when there is a cleaner way to do it
add_test(cleanup ruby test_output_cleanup.rb)

add_test(initialization ${test_executable_name} test-varnam-initialization td-00.vst)
add_test(transliteration ${test_executable_name} basic-transliteration td-01.vst)
add_test(ml-unicode ${test_executable_name} ml-unicode ../vsf/ml-unicode.vst ml-unicode-input.txt)
add_test(ml-unicode-reverse ${test_executable_name} ml-unicode-reverse ../vsf/ml-unicode.vst ml-unicode-reverse-input.txt)
add_test(test-vst-file-creation ${test_executable_name} test-vst-file-creation)
    

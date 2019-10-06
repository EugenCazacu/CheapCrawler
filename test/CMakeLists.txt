include(GoogleTest)

# --------
# add_test_with_properties will add the test with the following parameters:
# NAME <test_name> - the test name for the new test
# COMMAND <args>... - multiple args to be forwarded to the add_test command
# PROPERTIES <args>... - multiple args to be forwarded to the set_tests_properties command
# Example usage:
# add_test_with_properties(
#   NAME DriverCreateOutputDb
#   COMMAND ${DB_CREATOR} -c 1 -s 1 -d 1 --dbname ${DB_NAME} --dbOptions "port=${POSTGRES_PORT}"
#   PROPERTIES
#     FAIL_REGULAR_EXPRESSION ${ERROR_MSG_STRING}
#     FIXTURES_SETUP F_GeneralTestDbUp
#   )
# --------
function(add_test_with_properties)
  set(options GTEST)
  set(oneValueArgs NAME)
  set(multiValueArgs PROPERTIES COMMAND)
  cmake_parse_arguments(PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT PARAMS_NAME)
    message(FATAL_ERROR "test has no NAME")
  endif()

  #message("test: ${PARAMS_NAME} COMMAND: ${PARAMS_COMMAND}")

  if(PARAMS_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "test ${PARAMS_NAME} found unparsed argumets: ${PARAMS_UNPARSED_ARGUMENTS}")
  endif()

  if(PARAMS_GTEST)

    if(PARAMS_COMMAND)
      message(FATAL_ERROR "COMMAND cannot be used with GTEST. NAME: ${PARAMS_NAME}")
    endif()

    gtest_discover_tests(${PARAMS_NAME} TEST_PREFIX ${PARAMS_NAME}: PROPERTIES ${PARAMS_PROPERTIES})

  else()

    if(NOT PARAMS_COMMAND)
      message(FATAL_ERROR "test ${PARAMS_NAME} has no COMMAND")
    endif()

    add_test(${PARAMS_NAME} ${PARAMS_COMMAND})

    if(PARAMS_PROPERTIES)
      set_tests_properties(${PARAMS_NAME} PROPERTIES ${PARAMS_PROPERTIES})
    endif()

  endif()
endfunction()

add_subdirectory(crawler)


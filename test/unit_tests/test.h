/**-----------------------------------------------------------------------------
; @file test.h
;
; @language C99
;
; @brief
;   The file contains functionality for writing unit tests.
;
;   Usage:
;     Define TEST_RUN preprocessor macro to be any non-zero value.
;     Create c/cpp file for writing tests.
;     Include this header file in the created c/cpp file.
;     Include file(s) with the functionality to be tested in the c/cpp file.
;     Describe test cases.
;       Testcase description syntax:
;         - Test cases begin with the TEST_BEGIN statement, followed by the name
;           of the test case in parentheses.
;         - Test cases end with TEST_END statement.
;     Write RUN_TESTS followed by the names of the test cases to be executed
;     in parentheses.
;
;   Example:
;
;       #include "test.h"
;       #include "unit_under_test.hpp"
;
;       TEST_BEGIN(sum_test)
;           int result = unit_under_test_sum(2, 2);
;           EXPECT(result, 4);
;       TEST_END
;
;       TEST_BEGIN(mul_test)
;           int result = unit_under_test_mul(10, 0);
;           EXPECT_ZERO(result);
;       TEST_END
;
;       RUN_TESTS(sum_test, mul_test);
;
;   Each test case described between TEST_BEGIN and TEST_END is a separate
;   function with all the consequences. The namespaces of each test case do not
;   overlap. All local objects are automatically deleted after the test case
;   ends. For dynamically allocated objects, the release of memory falls on the
;   shoulders of the one who writes the test.
;
;   Tests executed in the order they are specified in arguments for RUN_TESTS.
;   Anyway, results of good test cases should not depend on the order in which
;   they are executed.
;
;   The RUN_TESTS macro creates an entry point to the program. To build tests,
;   original entry point must be out of scope (for example, placed inside the
;   #ifndef TEST_RUN construct).
;
; @author ep1h
;-----------------------------------------------------------------------------*/
// TODO: Support unicode.
// TODO: Implement output in file.
// TODO: Implement global initialization function.
// TODO: Support entry points for Windows applications.
// TODO: Expectation macros for strings(data buffers).
// TODO: Implement support for multiple RUN_TESTS(...) calls.

#ifndef TEST_HPP
#define TEST_HPP

#ifndef TEST_RUN
#error This file should not be included in non-test builds. Define the build as\
 a test build (define the "TEST_RUN" preprocessor macro to a non-zero value),\
 or exclude this file file from the build.
#endif

/* Provide access to private and protected members of classes */
#ifdef __cplusplus
#define private public
#define protected public
#endif

#include <stdio.h>

#define PRINTF PRINTF_A
#define OUTPUT WRITE_IN_CONSOLE

#define PRINTF_A printf

#define WRITE_IN_CONSOLE(format, ...)                                          \
    do                                                                         \
    {                                                                          \
        PRINTF(format, ##__VA_ARGS__);                                         \
    } while (0)

#define TEST_PASS 0
#define TEST_FAIL 1

#define TOTAL_EXPECTEDS total_expecteds
#define FAILED_EXPECTEDS failed_expecteds
#define TEST_RESULT result

typedef struct stTestCaseInfo
{
    int TOTAL_EXPECTEDS;
    int FAILED_EXPECTEDS;
    int TEST_RESULT;
} stTestCaseInfo;

#define TEST_BEGIN(TEST_NAME)                                                  \
    static void TEST_NAME(stTestCaseInfo *ti)                                  \
    {                                                                          \
        int TOTAL_EXPECTEDS = 0;                                               \
        int FAILED_EXPECTEDS = 0;                                              \
        int TEST_RESULT = 0;                                                   \
        OUTPUT("Executing test \'%s\'...\n", #TEST_NAME);

#define TEST_FOOTER                                                            \
    do                                                                         \
    {                                                                          \
        ti->TOTAL_EXPECTEDS = TOTAL_EXPECTEDS;                                 \
        ti->FAILED_EXPECTEDS = FAILED_EXPECTEDS;                               \
        ti->TEST_RESULT = TEST_RESULT;                                         \
    } while (0);

#define TEST_END                                                               \
    TEST_FOOTER;                                                               \
    return;                                                                    \
    }

#define EXPECT(value, expected)                                                \
    do                                                                         \
    {                                                                          \
        ++TOTAL_EXPECTEDS;                                                     \
        if (value != expected)                                                 \
        {                                                                      \
            ++FAILED_EXPECTEDS;                                                \
            TEST_RESULT = TEST_FAIL;                                           \
        }                                                                      \
    } while (false)

#define EXPECT_ZERO(value) EXPECT((value == 0), 1)

#define EXPECT_NOT_ZERO(value) EXPECT((value == 0), 0)

#define FORCE_FAIL_TEST                                                        \
    TEST_RESULT = TEST_FAIL;                                                   \
    TEST_FOOTER;                                                               \
    return;

#define TESTS_SEPARATOR                                                        \
    "------------------------------------------------------------------------" \
    "--------\n"

#define RUN_TESTS(...)                                                         \
    int main(int argc, char *argv[])                                           \
    {                                                                          \
        unsigned int failed_testcases = 0;                                     \
        void (*tests_array[])(stTestCaseInfo *) = {__VA_ARGS__};               \
        stTestCaseInfo ti = {0};                                               \
        for (unsigned int i = 0; i < sizeof(tests_array) / sizeof(void *);     \
             i++)                                                              \
        {                                                                      \
            tests_array[i](&ti);                                               \
            if (ti.TEST_RESULT == TEST_PASS)                                   \
            {                                                                  \
                OUTPUT("PASSED ");                                             \
            }                                                                  \
            else                                                               \
            {                                                                  \
                OUTPUT("FAILED ");                                             \
                ++failed_testcases;                                            \
            }                                                                  \
            OUTPUT("(%d/%d)\n", ti.TOTAL_EXPECTEDS - ti.FAILED_EXPECTEDS,      \
                   ti.TOTAL_EXPECTEDS);                                        \
            OUTPUT(TESTS_SEPARATOR);                                           \
        }                                                                      \
        OUTPUT("Executed tests: %d (%d passed, %d failed).\n",                 \
               sizeof(tests_array) / sizeof(void *),                           \
               sizeof(tests_array) / sizeof(void *) - failed_testcases,        \
               failed_testcases);                                              \
        return failed_testcases;                                               \
    }

#endif /* TEST_HPP */

/**-----------------------------------------------------------------------------
; @file unit_tests.cpp
;
; @brief
;   The file contains tests for ExternalProcess class.
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#include "test.h"

#include "../../../src/external_process.hpp"

#define QQ(x) #x
#define Q(x) QQ(x)

#ifndef EXTERNAL_PROCESS_SIMULATOR_NAME
#define EXTERNAL_PROCESS_SIMULATOR_NAME external_process_simulator.exe
#endif /* EXTERNAL_PROCESS_SIMULATOR_NAME */

/* Remote process name. Testing functionality of ExternalProcess class will be
 * performed based on interaction with this process. */
static const char test_application[] = Q(EXTERNAL_PROCESS_SIMULATOR_NAME);

using P1ExternalProcess::ExternalProcess;

TEST_BEGIN(get_process_id_by_non_existent_process_name)
{
    ExternalProcess ep((uint32_t)0);
    EXPECT(ep.get_process_id_by_process_name("!@#$^&*()_+"), 0);
}
TEST_END

TEST_BEGIN(get_process_id_by_existent_process_name)
{
    ExternalProcess ep((uint32_t)0);
    EXPECT_NOT_ZERO(ep.get_process_id_by_process_name(test_application));
}
TEST_END

RUN_TESTS(get_process_id_by_non_existent_process_name,
          get_process_id_by_existent_process_name);

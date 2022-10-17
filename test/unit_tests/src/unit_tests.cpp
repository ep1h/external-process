/**-----------------------------------------------------------------------------
; @file unit_tests.cpp
;
; @brief
;   The file contains tests for ExternalProcess class.
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#include "test.h"
#include "test_utils.hpp"
#include "../../../src/external_process.hpp"

using P1ExternalProcess::ExternalProcess;

TEST_BEGIN(get_process_id_by_non_existent_process_name)
    ExternalProcess ep((uint32_t)0);
    EXPECT(ep.get_process_id_by_process_name("!@#$^&*()_+"), 0);
TEST_END

TEST_BEGIN(get_process_id_by_existent_process_name)
    run_external_process_simulator();
    ExternalProcess ep((uint32_t)0);
    EXPECT_NOT_ZERO(ep.get_process_id_by_process_name(test_application));
    terminate_external_process_simulator();
TEST_END

TEST_BEGIN(read_buf)
    uint32_t buf_addr = run_external_process_simulator("buffer");
    uint32_t buf_size = run_external_process_simulator("buffer_size");
    uint8_t *buf = new uint8_t[buf_size];
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    ep.read_buf(buf_addr, buf_size, buf);
    for (unsigned int i = 0; i < buf_size; i++)
    {
        EXPECT(buf[i], i);
    }
    terminate_external_process_simulator();
    delete[] buf;
TEST_END

RUN_TESTS(get_process_id_by_non_existent_process_name,
          get_process_id_by_existent_process_name, read_buf);

/**-----------------------------------------------------------------------------
; @file unit_tests.cpp
;
; @brief
;   The file contains tests for ExternalProcess class.
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#include <cstring>
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

TEST_BEGIN(write_buf)
    uint32_t buf_addr = run_external_process_simulator("buffer");
    uint32_t buf_size = run_external_process_simulator("buffer_size");
    uint8_t *buf = new uint8_t[buf_size];
    memset(buf, 0xBB, buf_size);
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    ep.write_buf(buf_addr, buf_size, buf);
    memset(buf, 0, buf_size);
    ep.read_buf(buf_addr, buf_size, buf);
    for (unsigned int i = 0; i < buf_size; i++)
    {
        // OUTPUT("%x ", buf[i]);
        EXPECT(buf[i], 0xBB);
    }
    terminate_external_process_simulator();
    delete[] buf;
TEST_END

TEST_BEGIN(alloc_free)
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    EXPECT(ep._allocated_memory.size(), 0);
    uint32_t result_1 = ep.alloc(64);
    EXPECT(ep._allocated_memory.size(), 1);
    uint32_t result_2 = ep.alloc(32);
    EXPECT(ep._allocated_memory.size(), 2);
    uint32_t result_3 = ep.alloc(16);
    EXPECT(ep._allocated_memory.size(), 3);

    /* Alloc 0 bytes */
    uint32_t result_4 = ep.alloc(0);
    EXPECT(result_4, 0);
    EXPECT(ep._allocated_memory.size(), 3);

    /* Alloc too many bytes */
    uint32_t result_5 = ep.alloc(-1);
    EXPECT(result_5, 0);
    EXPECT(ep._allocated_memory.size(), 3);

    /* Free unallocated memory */
    ep.free(0);
    EXPECT(ep._allocated_memory.size(), 3);

    EXPECT(ep._allocated_memory.at(result_1), 64);
    EXPECT(ep._allocated_memory.at(result_2), 32);
    EXPECT(ep._allocated_memory.at(result_3), 16);
    ep.free(result_2);
    EXPECT(ep._allocated_memory.size(), 2);
    ep.free(result_1);
    ep.free(result_3);
    EXPECT(ep._allocated_memory.size(), 0);
    terminate_external_process_simulator();
TEST_END

TEST_BEGIN(cdecl_caller)
    uint32_t cdecl_sum_func_addr = run_external_process_simulator("sum_cdecl");
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    uint32_t sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 2, 4);
    EXPECT(sum, 6);
    terminate_external_process_simulator();
TEST_END

TEST_BEGIN(stdcall_caller)
    uint32_t stdcall_sum_func_addr =
        run_external_process_simulator("sum_stdcall");
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    uint32_t sum = ep.call_stdcall_function(stdcall_sum_func_addr, 2, 4, 3);
    EXPECT(sum, 7);
    terminate_external_process_simulator();
TEST_END

TEST_BEGIN(thiscall_caller)
    uint32_t thiscall_sum_func_addr =
        run_external_process_simulator("sum_thiscall");
    uint32_t class_obj_address = run_external_process_simulator("obj");
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    uint32_t sum = ep.call_thiscall_function(thiscall_sum_func_addr,
                                             class_obj_address, 2, 5, 5);
    EXPECT(sum, 10);
    terminate_external_process_simulator();
TEST_END

TEST_BEGIN(jmp_injector)
    uint32_t cdecl_sum_func_addr = run_external_process_simulator("sum_cdecl");
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    /* Add 1 to each argument */
    uint8_t injected_bytes[] =
    {
        0x83, 0x44, 0x24, 0x04, 0x01, /* add DWORD PTR [esp0x4],0x1 */
        0x83, 0x44, 0x24, 0x08, 0x01, /* add DWORD PTR [esp0x8],0x1 */
    };
    ep.inject_code_using_jmp(cdecl_sum_func_addr, injected_bytes,
                             sizeof(injected_bytes), 6);
    uint32_t sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 0, 0);
    EXPECT(sum, 2);
    sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 6, 4);
    EXPECT(sum, 12);
    terminate_external_process_simulator();
TEST_END

TEST_BEGIN(push_ret_injector)
    uint32_t stdcall_sum_func_addr =
        run_external_process_simulator("sum_stdcall");
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    /* Add 2 to each argument */
    uint8_t injected_bytes[] =
    {
        0x83, 0x44, 0x24, 0x04, 0x02, /* add DWORD PTR [esp0x4],0x2 */
        0x83, 0x44, 0x24, 0x08, 0x02, /* add DWORD PTR [esp0x8],0x2 */
    };
    ep.inject_code_using_push_ret(stdcall_sum_func_addr, injected_bytes,
                                  sizeof(injected_bytes), 6);
    uint32_t sum = ep.call_stdcall_function(stdcall_sum_func_addr, 2, 0, 0);
    EXPECT(sum, 4);
    sum = ep.call_stdcall_function(stdcall_sum_func_addr, 2, 1, 2);
    EXPECT(sum, 7);
    terminate_external_process_simulator();
TEST_END

RUN_TESTS(get_process_id_by_non_existent_process_name,
          get_process_id_by_existent_process_name, read_buf, write_buf,
          alloc_free, cdecl_caller, stdcall_caller, thiscall_caller,
          jmp_injector, push_ret_injector);

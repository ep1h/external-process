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
{
    ExternalProcess ep((uint32_t)0);
    EXPECT(ep.get_process_id_by_process_name("!@#$^&*()_+"), 0);
}
TEST_END

TEST_BEGIN(get_process_id_by_existent_process_name)
{
    run_external_process_simulator();
    ExternalProcess ep((uint32_t)0);
    EXPECT_NOT_ZERO(ep.get_process_id_by_process_name(test_application));
    terminate_external_process_simulator();
}
TEST_END

TEST_BEGIN(read_buf)
{
    uint32_t buf_addr = get_sim_info()->buffer_address;
    uint32_t buf_size = get_sim_info()->buffer_size;
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
}
TEST_END

TEST_BEGIN(write_buf)
{
    uint32_t buf_addr = get_sim_info()->buffer_address;
    uint32_t buf_size = get_sim_info()->buffer_size;
    uint8_t *buf = new uint8_t[buf_size];
    memset(buf, 0xBB, buf_size);
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    ep.write_buf(buf_addr, buf_size, buf);
    memset(buf, 0, buf_size);
    ep.read_buf(buf_addr, buf_size, buf);
    for (unsigned int i = 0; i < buf_size; i++)
    {
        EXPECT(buf[i], 0xBB);
    }
    terminate_external_process_simulator();
    delete[] buf;
}
TEST_END

TEST_BEGIN(alloc_free)
{
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
}
TEST_END

TEST_BEGIN(cdecl_caller)
{
    uint32_t cdecl_sum_func_addr = get_sim_info()->sum_cdecl_function_address;
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    uint32_t sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 2, 4);
    EXPECT(sum, 6);
    terminate_external_process_simulator();
}
TEST_END

TEST_BEGIN(stdcall_caller)
{
    uint32_t stdcall_sum_func_addr =
        get_sim_info()->sum_stdcall_function_address;
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    uint32_t sum = ep.call_stdcall_function(stdcall_sum_func_addr, 2, 4, 3);
    EXPECT(sum, 7);
    terminate_external_process_simulator();
}
TEST_END

TEST_BEGIN(thiscall_caller)
{
    uint32_t thiscall_sum_func_addr =
        get_sim_info()->sum_thiscall_function_address;
    uint32_t class_obj_address = get_sim_info()->class_object_address;
    run_external_process_simulator();
    ExternalProcess ep(test_application);
    uint32_t sum = ep.call_thiscall_function(thiscall_sum_func_addr,
                                             class_obj_address, 2, 5, 5);
    EXPECT(sum, 10);
    terminate_external_process_simulator();
}
TEST_END

TEST_BEGIN(jmp_injector)
{
    uint32_t cdecl_sum_func_addr = get_sim_info()->sum_cdecl_function_address;
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    /* Add 1 to each argument */
    uint8_t injected_bytes[] = {
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
}
TEST_END

TEST_BEGIN(push_ret_injector)
{
    uint32_t stdcall_sum_func_addr =
        get_sim_info()->sum_stdcall_function_address;
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    /* Add 2 to each argument */
    uint8_t injected_bytes[] = {
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
}
TEST_END

TEST_BEGIN(uninject)
{
    uint32_t cdecl_sum_func_addr = get_sim_info()->sum_cdecl_function_address;
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    /* Add 5 to each argument */
    uint8_t injected_bytes[] = {
        0x83, 0x44, 0x24, 0x04, 0x05, /* add DWORD PTR [esp0x4],0x5 */
        0x83, 0x44, 0x24, 0x08, 0x05, /* add DWORD PTR [esp0x8],0x5 */
    };
    ep.inject_code(cdecl_sum_func_addr, injected_bytes, sizeof(injected_bytes),
                   6, P1ExternalProcess::enInjectionType::EIT_JMP);
    uint32_t sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 0, 0);
    EXPECT(sum, 10);
    sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 6, 4);
    EXPECT(sum, 20);
    ep.uninject_code(cdecl_sum_func_addr);
    sum = ep.call_cdecl_function(cdecl_sum_func_addr, 2, 6, 4);
    EXPECT(sum, 10);
    terminate_external_process_simulator();
}
TEST_END

TEST_BEGIN(signature_scanner)
{
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    uint32_t buf_address = get_sim_info()->buffer_address;
    uint32_t buf_size = get_sim_info()->buffer_size;

    /* Search for signature in the begin */
    const char *mask_1 = "xxxx";
    const uint8_t sig_1[] = {0x00, 0x01, 0x02, 0x03};
    uint32_t res = ep.find_signature(buf_address, buf_size, sig_1, mask_1);
    EXPECT(res, buf_address);

    /* Search for signature in the end */
    const uint8_t sig_2[] = {0x1C, 0x1D, 0x1E, 0x1F};
    res = ep.find_signature(buf_address, buf_size, sig_2, mask_1);
    EXPECT(res, buf_address + buf_size - 4);

    /* Search for signature in the ~middle */
    const uint8_t sig_3[] = {0x10, 0x11, 0x12, 0x13};
    res = ep.find_signature(buf_address, buf_size, sig_3, mask_1);
    EXPECT(res, buf_address + 0x10);

    /* Search for false signature */
    const uint8_t sig_4[] = {0xFF, 0xBA, 0xDD, 0x11};
    res = ep.find_signature(buf_address, buf_size, sig_4, mask_1);
    EXPECT(res, 0);

    /* Search for signature ignoring bytes by the mask */
    const uint8_t sig_5[] = {0x05, 0xBA, 0xAD, 0x08};
    const char *mask_2 = "x??x";
    res = ep.find_signature(buf_address, buf_size, sig_5, mask_2);
    EXPECT(res, buf_address + 0x05);

    /* Use the previous signature, but mask is shorter than signature */
    const char *mask_3 = "x??";
    res = ep.find_signature(buf_address, buf_size, sig_5, mask_3);
    EXPECT(res, buf_address + 0x05);

    /* Use the previous signature, but mask is longer than signature */
    const char *mask_4 = "x??xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    res = ep.find_signature(buf_address, buf_size, sig_5, mask_4);
    EXPECT(res, 0);

    /* Nulls */
    res = ep.find_signature(buf_address, buf_size, 0, mask_1);
    EXPECT(res, 0);
    res = ep.find_signature(buf_address, buf_size, sig_3, 0);
    EXPECT(res, 0);
    res = ep.find_signature(buf_address, 0, sig_3, mask_1);
    EXPECT(res, 0);

    terminate_external_process_simulator();
}
TEST_END

TEST_BEGIN(get_module_name)
{
    run_external_process_simulator();
    ExternalProcess ep(test_application);

    EXPECT_NOT_ZERO(ep.get_module_address(test_application));
    EXPECT_NOT_ZERO(ep.get_module_address("ntdll.dll"));
    EXPECT_ZERO(ep.get_module_address("!@#$%^&*()_"));
    EXPECT_ZERO(ep.get_module_address(""));
    EXPECT_ZERO(ep.get_module_address(NULL));

    terminate_external_process_simulator();
}
TEST_END

RUN_TESTS(get_process_id_by_non_existent_process_name,
          get_process_id_by_existent_process_name, read_buf, write_buf,
          alloc_free, cdecl_caller, stdcall_caller, thiscall_caller,
          jmp_injector, push_ret_injector, uninject, signature_scanner,
          get_module_name);

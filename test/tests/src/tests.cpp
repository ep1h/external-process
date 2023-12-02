#include <iostream>
#include "test_utils.hpp"
#include "../eztest/eztest.h"
#include "../../../src/core/ep_core.h"

static char sim_name_[] = "external_process_simulator.exe";
static SharedData sd_;

TEST_BEGIN(epc_create_destroy_test)
{
    EXPECT(epc_create(NULL), false);
    EXPECT(epc_destroy(NULL), false);
    EPCProcess* process = NULL;
    EXPECT(epc_create(&process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(epc_get_process_id_by_name_test)
{
    EXPECT(epc_get_process_id_by_name(NULL, NULL), false);
    EXPECT(epc_get_process_id_by_name(sim_name_, NULL), false);
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(NULL, &process_id), false);
    EXPECT(epc_get_process_id_by_name(" !@#$%^&*()_+ ", &process_id), false);
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
}
TEST_END

TEST_BEGIN(epc_open_process_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(0, 0), false);
    EXPECT(epc_open_process(process, 0), false);
    EXPECT(epc_open_process(process, process_id), true);
    EXPECT(epc_close_process(0), false);
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(epc_read_buf_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    uint8_t* sim_buf = new uint8_t[sd_.buf_size];
    char buf[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    EXPECT(epc_open_process(process, process_id), true);
    EXPECT(epc_read_buf(NULL, sd_.buf_addr, sd_.buf_size, sim_buf), false);
    EXPECT(epc_read_buf(process, sd_.buf_addr, sd_.buf_size, 0), false);
    EXPECT(epc_read_buf(process, sd_.buf_addr, sd_.buf_size, sim_buf), true);
    EXPECT_BUF(sim_buf, buf, sizeof(buf));
    delete[] sim_buf;
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END


TEST_BEGIN(epc_write_buf_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    char buf[] = {0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
                  0x17, 0x16, 0x15, 0x15, 0x13, 0x12, 0x11, 0x10,
                  0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
                  0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    EXPECT(epc_open_process(process, process_id), true);
    EXPECT(epc_write_buf(NULL, sd_.buf_addr, sd_.buf_size, buf), false);
    EXPECT(epc_write_buf(process, sd_.buf_addr, sd_.buf_size, 0), false);
    EXPECT(epc_write_buf(process, sd_.buf_addr, sd_.buf_size, buf), true);
    uint8_t* sim_buf = new uint8_t[sd_.buf_size];
    EXPECT(epc_read_buf(process, sd_.buf_addr, sd_.buf_size, sim_buf), true);
    EXPECT_BUF(sim_buf, buf, sizeof(buf));
    delete[] sim_buf;
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(epc_alloc_epc_free_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    uintptr_t addr = 0;
    uintptr_t buf[10];
    for (size_t i = 0; i < sizeof(buf) / sizeof(buf[0]); i++)
    {
        buf[i] = i;
    }
    EXPECT(epc_alloc(NULL, sizeof(buf), &addr), false);
    EXPECT(addr, 0);
    EXPECT(epc_alloc(process, sizeof(buf), NULL), false);
    EXPECT(addr, 0);
    EXPECT(epc_alloc(process, sizeof(buf), &addr), true);
    EXPECT(addr == 0, false);
    EXPECT(epc_write_buf(process, addr, sizeof(buf), buf), true);
    for (size_t i = 0; i < sizeof(buf) / sizeof(buf[0]); i++)
    {
        buf[i] = 0;
    }
    EXPECT(epc_read_buf(process, addr, sizeof(buf), buf), true);
    for (size_t i = 0; i < sizeof(buf) / sizeof(buf[0]); i++)
    {
        EXPECT(buf[i], i);
    }
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(epc_virtual_protect_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    EPCVirtualProtect old_protect;

    EXPECT(
        epc_virtual_protect(NULL, sd_.buf_addr, sd_.buf_size, EPCVP_READ, NULL),
        false);
    EXPECT(epc_virtual_protect(process, sd_.buf_addr, sd_.buf_size, EPCVP_EMPTY,
                               &old_protect),
           false);

    /* Set read-only protection. */
    EXPECT(epc_virtual_protect(process, sd_.buf_addr, sd_.buf_size, EPCVP_READ,
                               &old_protect),
           true);
    /* Try to write to the buffer. */
    uint8_t buf[5] = {1, 2, 3, 4, 5};
    EXPECT(epc_write_buf(process, sd_.buf_addr, sizeof(buf), buf), false);
    /* Restore old protection. */
    EPCVirtualProtect tmp_protect;
    EXPECT(epc_virtual_protect(process, sd_.buf_addr, sd_.buf_size, old_protect,
                               &tmp_protect),
           true);
    /* Try to write to the buffer again. */
    EXPECT(epc_write_buf(process, sd_.buf_addr, sizeof(buf), buf), true);

    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(epc_get_module_address_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    uintptr_t module_address = 0;
    EXPECT(epc_get_module_address(NULL, "ntdll.dll", &module_address), false);
    EXPECT(epc_get_module_address(process, "ntdll.dll", NULL), false);
    EXPECT(epc_get_module_address(process, NULL, &module_address), false);
    EXPECT(epc_get_module_address(process, " !@#$%^&*()_+ ", &module_address),
           true);
    EXPECT(epc_get_module_address(process, "ntdll.dll", &module_address), true);
    EXPECT(module_address == 0, false);
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(epc_function_caller_build_epc_function_caller_destroy_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    EPCFunctionCaller* caller;
    EXPECT(epc_function_caller_build(NULL, sd_.sum_cdecl_addr, EPCCC_CDECL, 2,
                                     &caller),
           false);
    EXPECT(epc_function_caller_build(process, sd_.sum_cdecl_addr, EPCCC_CDECL,
                                     2, NULL),
           false);
    EXPECT(epc_function_caller_build(process, sd_.sum_cdecl_addr, EPCCC_INVALID,
                                     2, &caller),
           false);
    EXPECT(epc_function_caller_build(process, sd_.sum_cdecl_addr, EPCCC_CDECL,
                                     2, &caller),
           true);

    EXPECT(epc_function_caller_destroy(NULL), false);
    EXPECT(epc_function_caller_destroy(caller), true);
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(cdecl_caller_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    EPCFunctionCaller* caller;
    EXPECT(epc_function_caller_build(process, sd_.sum_cdecl_addr, EPCCC_CDECL,
                                     2, &caller),
           true);
    EXPECT(epc_function_caller_send_args(caller, 2, 3, 4), true);
    EXPECT(epc_function_caller_send_args(NULL, 2, 3, 4), false);
    EXPECT(epc_function_caller_send_args(caller, 1, 7), false);
    EXPECT(epc_function_caller_send_args(caller, 3, 7, 8, 9), false);
    uintptr_t result = 0;
    EXPECT(epc_function_caller_call(caller, &result, true), true);
    EXPECT(epc_function_caller_call(NULL, &result, true), false);
    EXPECT(epc_function_caller_call(caller, NULL, true), false);
    EXPECT(result, 7);
    EXPECT(epc_function_caller_send_args(caller, 2, 0xFFFFFFF0, 0xF), true);
    EXPECT(epc_function_caller_call(caller, &result, true), true);
    EXPECT(result, 0xFFFFFFFF);
    EXPECT(epc_function_caller_destroy(NULL), false);
    EXPECT(epc_function_caller_destroy(caller), true);
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(stdcall_caller_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    EPCFunctionCaller* caller;
    EXPECT(epc_function_caller_build(process, sd_.sum_stdcall_addr,
                                     EPCCC_STDCALL, 2, &caller),
           true);
    EXPECT(epc_function_caller_send_args(caller, 2, 7, 8), true);
    EXPECT(epc_function_caller_send_args(NULL, 2, 4, 5), false);
    uintptr_t result = 0;
    EXPECT(epc_function_caller_call(caller, &result, true), true);
    EXPECT(epc_function_caller_call(NULL, &result, true), false);
    EXPECT(epc_function_caller_call(caller, NULL, true), false);
    EXPECT(result, 15);
    EXPECT(epc_function_caller_send_args(caller, 2, 0xFFFFFFFE, 0x1), true);
    EXPECT(epc_function_caller_call(caller, &result, true), true);
    EXPECT(result, 0xFFFFFFFF);
    EXPECT(epc_function_caller_destroy(NULL), false);
    EXPECT(epc_function_caller_destroy(caller), true);
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END

TEST_BEGIN(cdecl_thiscall_test)
{
    EPCProcess* process = NULL;
    unsigned int process_id = 0;
    EXPECT(epc_get_process_id_by_name(sim_name_, &process_id), true);
    EXPECT(epc_create(&process), true);
    EXPECT(epc_open_process(process, process_id), true);
    EPCFunctionCaller* caller;
    EXPECT(epc_function_caller_build(process, sd_.sum_thiscall_addr,
                                     EPCCC_THISCALL_MSVC, 2, &caller),
           true);
    EXPECT(
        epc_function_caller_send_args(caller, 3, sd_.thiscall_obj_addr, 15, 21),
        true);
    EXPECT(epc_function_caller_send_args(NULL, 2, 3, 4), false);
    EXPECT(epc_function_caller_send_args(caller, 2, 3, 4), false);
    uintptr_t result = 0;
    EXPECT(epc_function_caller_call(NULL, &result, true), false);
    EXPECT(epc_function_caller_call(caller, NULL, true), false);
    EXPECT(epc_function_caller_call(caller, &result, true), true);
    EXPECT(result, 36);
    EXPECT(epc_function_caller_send_args(caller, 3, sd_.sum_thiscall_addr,
                                         0xFFFF0001, 0xFFFE),
           true);
    EXPECT(epc_function_caller_call(caller, &result, true), true);
    EXPECT(result, 0xFFFFFFFF);
    EXPECT(epc_function_caller_destroy(NULL), false);
    EXPECT(epc_function_caller_destroy(caller), true);
    EXPECT(epc_close_process(process), true);
    EXPECT(epc_destroy(process), true);
}
TEST_END


RUN_TESTS(epc_core_tests, epc_create_destroy_test,
          epc_get_process_id_by_name_test, epc_open_process_test,
          epc_read_buf_test, epc_write_buf_test, epc_alloc_epc_free_test,
          epc_virtual_protect_test, epc_get_module_address_test,
          epc_function_caller_build_epc_function_caller_destroy_test,
          cdecl_caller_test, stdcall_caller_test, cdecl_thiscall_test);

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    std::string sim_path = argv[1];
    uint32_t res = run_executable(sim_path.c_str(), nullptr, true);
    sd_ = get_sim_info();
    epc_core_tests();
    terminate_external_process_simulator();
    return res;
}

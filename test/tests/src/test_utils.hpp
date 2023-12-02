#ifndef TEST_UTILS_HPP_
#define TEST_UTILS_HPP_

#include <stdbool.h>
#include <stdint.h>

struct SharedData
{
    uintptr_t sum_cdecl_addr;
    uintptr_t sum_stdcall_addr;
    uintptr_t sum_thiscall_addr;
    uintptr_t thiscall_obj_addr;
    uintptr_t buf_addr;
    size_t buf_size;
};

uint32_t run_executable(const char* app, const char* args, bool background);
bool terminate_external_process_simulator(void);
const SharedData get_sim_info(void);

#endif /* TEST_UTILS_HPP_ */

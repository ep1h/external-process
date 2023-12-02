/**
 * @file external_process_simulator.cpp
 *
 * @brief The file contains implementation of application used as an external
 * process to test functionality of ep_core functionality.
 */
#include <iostream>
#include <windows.h>
#include "../../shared_memory/src/shared_memory.hpp"
#include "c_cpp_compat.h"

/**
 * @brief The structure contains information shared between this process and
 * tests.
 */
struct SharedData
{
    uintptr_t sum_cdecl_addr;
    uintptr_t sum_stdcall_addr;
    uintptr_t sum_thiscall_addr;
    uintptr_t thiscall_obj_addr;
    uintptr_t buf_addr;
    size_t buf_size;
};

char buffer[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

NOINLINE_ CC_CDECL_ int sum_cdecl(int a, int b)
{
    int result = a + b;
    std::cout << "sum_cdecl(" << a << ", " << b << ") = " << result
              << std::endl;
    return result;
}

NOINLINE_ CC_STDCALL_ int sum_stdcall(int a, int b)
{
    int result = a + b;
    std::cout << "sum_stdcall(" << a << ", " << b << ") = " << result
              << std::endl;
    return result;
}

class Class
{
public:
    NOINLINE_ int sum_thiscall(int a, int b)
    {
        int result = a + b;
        std::cout << "sum_thiscall(" << a << ", " << b << ") = " << result
                  << std::endl;
        return result;
    }

private:
    int var_;
};

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    SharedMemory<SharedData> shared_data("ExternalProcessSimulatorSharedData");
    shared_data.get()->sum_cdecl_addr = reinterpret_cast<uintptr_t>(&sum_cdecl);
    shared_data.get()->sum_stdcall_addr =
        reinterpret_cast<uintptr_t>(&sum_stdcall);
    auto sum_thiscall_ptr = &Class::sum_thiscall;
    shared_data.get()->sum_thiscall_addr =
        reinterpret_cast<uintptr_t&>(sum_thiscall_ptr);
    shared_data.get()->buf_addr = reinterpret_cast<uintptr_t>(&buffer);
    shared_data.get()->buf_size = sizeof(buffer);
    volatile Class obj;
    shared_data.get()->thiscall_obj_addr = reinterpret_cast<uintptr_t>(&obj);
    std::getchar();
    return 0;
}

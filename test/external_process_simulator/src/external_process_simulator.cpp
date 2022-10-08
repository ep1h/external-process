/**-----------------------------------------------------------------------------
; @file external_process_simulator.cpp
;
; @brief
;   The file contains implementation of application used as an external process
;   to test functionality of ExternalProcess class.
;
; @usage
;   external_process_simulator.exe [{sum_cdecl,sum_stdcall,sum_thiscall,
;                                   buffer,buffer_size,obj,loop,wait_for_input}]
;
; @parameters
;   sum_cdecl       The program will terminate immediately and return the
;                   address of @sum_cdecl function.
;   sum_stdcall     The program will terminate immediately and return the
;                   address of @sum_stdcall function.
;   sum_thiscall    The program will terminate immediately and return the
;                   address of @sum_thiscall function.
;   buffer          The program will terminate immediately and return the
;                   address of @buffer array.
;   buffer_size     The program will terminate immediately and return the size
;                   of @buffer array.
;   obj             The program will immediately exit and return the address of
;                   @obj object.
;   loop            The program will start an infinite loop.
;   wait_for_input  The program will start and wait for any line to be entered
;                   (pressing Enter) to finish.
; @author ep1h
-----------------------------------------------------------------------------**/
#include <iostream>
#include <string>

#if !defined __GNUC__ && !defined _MSC_VER
#error Unsupported compiler
#endif

#if defined __GNUC__
#define P1_NOINLINE __attribute__((noinline))
#define P1_CDECL __attribute((cdecl))
#define P1_STDCALL __attribute((stdcall))
#else
#if defined _MSC_VER
#define P1_NOINLINE __declspec(noinline)
#define P1_CDECL __cdecl
#define P1_STDCALL __stdcall
#endif
#endif

char buffer[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

int P1_NOINLINE P1_CDECL sum_cdecl(int a, int b)
{
    int result = a + b;
    std::cout << "sum_cdecl(" << a << ", " << b << ") = " << result
              << std::endl;
    return result;
}

int P1_NOINLINE P1_STDCALL sum_stdcall(int a, int b)
{
    int result = a + b;
    std::cout << "sum_stdcall(" << a << ", " << b << ") = " << result
              << std::endl;
    return result;
}

class Class
{
public:
    int P1_NOINLINE sum_thiscall(int a, int b)
    {
        int result = a + b;
        std::cout << "sum_thiscall(" << a << ", " << b << ") = " << result
                  << std::endl;
        return result;
    }

private:
    int _var;
};

void print_help(void)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "\texternal_process_simulator.exe "
                 "[{sum_cdecl,sum_stdcall,sum_thiscall,buffer,buffer_size,obj,"
                 "loop,wait_for_input}]"
              << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "\tsum_cdecl - The program will terminate immediately and "
                 "return the address of 'sum_cdecl' function.\n\tsum_stdcall - "
                 "The program will terminate immediately and return the "
                 "address of 'sum_stdcall' function."
              << std::endl;
    std::cout << "\tsum_thiscall -The program will terminate immediately and "
                 "return the address of 'sum_thiscall' function."
              << std::endl;
    std::cout << "\tbuffer - The program will terminate immediately and return "
                 "the address of 'buffer' array."
              << std::endl;
    std::cout << "\tbuffer_size - The program will terminate immediately and "
                 "return the size of 'buffer' array."
              << std::endl;
    std::cout << "\tobj - The program will immediately exit and return the "
                 "address of 'obj' object."
              << std::endl;
    std::cout << "\tloop - The program will start an infinite loop."
              << std::endl;
    std::cout << "\twait_for_input - The program will start and wait for any "
                 "line to be entered (pressing Enter) to finish."
              << std::endl
              << std::endl;
    std::cout << "One of the above startup arguments must be passed to the "
                 "application."
              << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        print_help();
        return -1;
    }
    std::cout << std::hex;
    Class obj;
    auto sum_thiscall_ptr = &Class::sum_thiscall;

    if (argc == 2)
    {
        if (std::string("sum_cdecl") == argv[1])
        {
            std::cout << "sum_cdecl address: " << (void *)sum_cdecl
                      << std::endl;
            return (int)(void *)sum_cdecl;
        }
        else if (std::string("sum_stdcall") == argv[1])
        {
            std::cout << "sum_stdcall address: " << (void *)sum_stdcall
                      << std::endl;
            return (int)(void *)sum_stdcall;
        }
        else if (std::string("sum_thiscall") == argv[1])
        {
            std::cout << "sum_thiscall address: " << (void *&)(sum_thiscall_ptr)
                      << std::endl;
            return (int)(void *&)sum_thiscall_ptr;
        }
        else if (std::string("buffer") == argv[1])
        {
            std::cout << "buffer[" << sizeof(buffer)
                      << "] address: " << (void *)buffer << std::endl;
            return (int)buffer;
        }
        else if (std::string("buffer_size") == argv[1])
        {
            std::cout << "buffer size: " << sizeof(buffer) << std::endl;
            return (int)sizeof(buffer);
        }
        else if (std::string("obj") == argv[1])
        {
            std::cout << "Class object address: " << (void *)&obj << std::endl;
            return (int)(void *)&obj;
        }
        else if (std::string("wait_for_input") == argv[1])
        {
            std::cout << "Press Enter for exit." << std::endl;
            std::cin.get();
        }
        else if (std::string("loop") == argv[1])
        {
            std::cout << "Infinite loop begins..." << std::endl;
            while (true)
            {
            }
        }
    }
    return 0;
}

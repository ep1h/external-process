/**
 * @file external_process.cpp
 *
 * @brief ExternalProcess class implementation.
 *
 */
#include "external_process.hpp"
#include <windows.h>
#include <tlhelp32.h>

// #define SHOW_ERRORS

#ifdef SHOW_ERRORS
#include <iostream>
#include <string>
static std::string get_winapi_error_text(BOOL error_id);
#define SHOW_WINAPI_ERROR(error_code)                                          \
    std::cerr << std::endl << "Error: " << error_code << std::endl;            \
    std::cerr << " File: " << __FILE__ << std::endl;                           \
    std::cerr << " Line: " << std::dec << __LINE__ << std::endl;               \
    std::cerr << " Function: " << __func__ << std::endl;                       \
    std::cerr << " Message: " << get_winapi_error_text(error_code)             \
              << std::endl                                                     \
              << std::endl;

#define WINAPI_CALL(call)                                                      \
    call;                                                                      \
    do                                                                         \
    {                                                                          \
        DWORD ________last_error = GetLastError();                             \
        SetLastError(0);                                                       \
        if (________last_error)                                                \
        {                                                                      \
            SHOW_WINAPI_ERROR(________last_error);                             \
        }                                                                      \
    } while (0)
#else
#define WINAPI_CALL(call) call
#endif /* SHOW_ERRORS */

using namespace P1ExternalProcess;

typedef NTSTATUS(NTAPI *tNtReadVirtualMemory)(
    IN HANDLE ProcessHandle, IN PVOID BaseAddress, OUT PVOID buffer,
    IN ULONG NumberOfBytesRead, OUT PULONG NumberOfBytesReaded OPTIONAL);

typedef NTSTATUS(NTAPI *tNtWriteVirtualMemory)(
    IN HANDLE ProcessHandle, IN PVOID BaseAddress, IN PVOID buffer,
    IN ULONG NumberOfBytesToWrite, OUT PULONG NumberOfBytesWritten OPTIONAL);

typedef NTSTATUS(NTAPI *tNtAllocateVirtualMemory)(
    IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize, IN ULONG AllocationType, IN ULONG Protect);

typedef NTSTATUS(NTAPI *tNtFreeVirtualMemory)(IN HANDLE ProcessHandle,
                                              IN OUT PVOID *BaseAddress,
                                              IN OUT PSIZE_T RegionSize,
                                              IN ULONG FreeType);

typedef NTSTATUS(NTAPI *tNtProtectVirtualMemory)(
    IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress,
    IN OUT PULONG NumberOfBytesToProtect, IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection);

static tNtReadVirtualMemory NtReadVirtualMemory =
    (tNtReadVirtualMemory)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                 "NtReadVirtualMemory");

static tNtWriteVirtualMemory NtWriteVirtualMemory =
    (tNtWriteVirtualMemory)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                  "NtWriteVirtualMemory");

static tNtAllocateVirtualMemory NtAllocateVirtualMemory =
    (tNtAllocateVirtualMemory)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                     "NtAllocateVirtualMemory");

static tNtFreeVirtualMemory NtFreeVirtualMemory =
    (tNtFreeVirtualMemory)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                 "NtFreeVirtualMemory");

static tNtProtectVirtualMemory NtProtectVirtualMemory =
    (tNtProtectVirtualMemory)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                    "NtProtectVirtualMemory");

#ifdef SHOW_ERRORS
static std::string get_winapi_error_text(BOOL error_id)
{
    if (!error_id)
        return "";

    LPSTR error_msg_buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&error_msg_buffer, 0, NULL);
    std::string error_msg(error_msg_buffer, size);
    LocalFree(error_msg_buffer);
    return error_msg;
}
#endif /* SHOW_ERRORS */

ExternalProcess::ExternalProcess(uint32_t process_id) : _process_id(process_id)
{
    _handle = WINAPI_CALL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id));
}

ExternalProcess::ExternalProcess(const char *process_name)
    : ExternalProcess(get_process_id_by_process_name(process_name))
{
}

ExternalProcess::~ExternalProcess()
{
    for (auto &i : _callers)
    {
        free(i.second.caller_address); // TODO: Implement ~caller_destroyer.
    }
    for (auto &i : _injected_code)
    {
        uninject_code(i.first);
    }
    for (auto i = _allocated_memory.cbegin(), n = i;
         i != _allocated_memory.cend(); i = n)
    {
        ++n;
        free(i->first);
    }
    for (auto &i : _virtual_protect)
    {
        restore_virtual_protect(i.first);
    }
    WINAPI_CALL(CloseHandle((HANDLE)_handle));
}

void ExternalProcess::read_buf(uintptr_t address, size_t size,
                               void *out_result) const
{
    // ReadProcessMemory(static_cast<HANDLE>(_handle),
    //                   reinterpret_cast<LPCVOID>(address), out_result, size,
    //                   0);
    NtReadVirtualMemory(static_cast<HANDLE>(_handle),
                        reinterpret_cast<PVOID>(address), out_result, size, 0);
}

void ExternalProcess::write_buf(uintptr_t address, size_t size,
                                const void *data) const
{
    // WriteProcessMemory(static_cast<HANDLE>(_handle),
    //                    reinterpret_cast<LPVOID>(address), data, size, 0);

    NtWriteVirtualMemory(static_cast<HANDLE>(_handle),
                         reinterpret_cast<PVOID>(address),
                         const_cast<PVOID>(data), size, 0);
}

uintptr_t ExternalProcess::alloc(const size_t size)
{
    // TODO: Add rights as function argument.
    // void *address =
    //    VirtualAllocEx(static_cast<HANDLE>(_handle), NULL, size,
    //                   MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    void *address = NULL;
    SIZE_T sz = size;
    NtAllocateVirtualMemory(static_cast<HANDLE>(_handle), &address, 0, &sz,
                            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (address != NULL)
    {
        _allocated_memory[reinterpret_cast<uintptr_t>(address)] = size;
    }
    return reinterpret_cast<uintptr_t>(address);
}

void ExternalProcess::free(uintptr_t address)
{
    auto result = _allocated_memory.find(address);
    if (result != _allocated_memory.end())
    {
        // VirtualFreeEx((HANDLE)_handle, reinterpret_cast<void *>(address),
        //               result->second, MEM_RELEASE);
        PVOID addr = reinterpret_cast<PVOID>(address);
        SIZE_T sz = 0;
        NtFreeVirtualMemory(static_cast<HANDLE>(_handle), &addr, &sz,
                            MEM_RELEASE);
        _allocated_memory.erase(address);
        _virtual_protect.erase(address);
    }
}

void ExternalProcess::set_virtual_protect(uintptr_t address, size_t size,
                                          enVirtualProtect type)
{
    ULONG old_protect = 0;
    PVOID addr = reinterpret_cast<PVOID>(address);
    ULONG sz = size;
    ULONG protect = 0;
    if (type == (enVirtualProtect::READ | enVirtualProtect::WRITE |
                 enVirtualProtect::EXECUTE))
    {
        protect = PAGE_READWRITE;
    }
    else if (type == (enVirtualProtect::READ & enVirtualProtect::WRITE))
    {
        protect = PAGE_READWRITE; // TODO: Not PAGE_READWRITE
    }
    else if (type == (enVirtualProtect::READ & enVirtualProtect::EXECUTE))
    {
        protect = PAGE_EXECUTE_READ;
    }
    else if (type == enVirtualProtect::READ)
    {
        protect = PAGE_READONLY;
    }
    else if (type == enVirtualProtect::NOACCESS)
    {
        protect = PAGE_NOACCESS;
    }

    NtProtectVirtualMemory(static_cast<HANDLE>(_handle), &addr, &sz, protect,
                           &old_protect);

    auto original_vp = _virtual_protect.find(address);
    if (original_vp == _virtual_protect.end())
    {
        _virtual_protect[address] = {size, old_protect};
    }
    else if (original_vp->second.size < size)
    {
        original_vp->second.size = size;
    }
}

void ExternalProcess::restore_virtual_protect(uintptr_t address)
{
    auto vp = _virtual_protect.find(address);
    if (vp != _virtual_protect.end())
    {
        ULONG old_protect;
        PVOID addr = reinterpret_cast<PVOID>(address);
        ULONG sz;
        NtProtectVirtualMemory(static_cast<HANDLE>(_handle), &addr, &sz,
                               vp->second.original_protect, &old_protect);
        _virtual_protect.erase(address);
    }
}

uintptr_t ExternalProcess::get_module_address(const char *module_name)
{
    // TODO: Store modules addresses in map.
    uintptr_t result = 0;
    HANDLE snapshot_handle = WINAPI_CALL(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, _process_id));
    if (snapshot_handle != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 module_entry;
        module_entry.dwSize = sizeof(module_entry);
        if (Module32First(snapshot_handle, &module_entry))
        {
            do
            {
                if (!_strcmpi(module_entry.szModule, module_name))
                {
                    result =
                        reinterpret_cast<uintptr_t>(module_entry.modBaseAddr);
                    break;
                }
            } while (Module32Next(snapshot_handle, &module_entry));
        }
    }
    WINAPI_CALL(CloseHandle(snapshot_handle));
    return result;
}

uintptr_t ExternalProcess::call_cdecl_function(uintptr_t address, size_t argc,
                                               ...)
{
    if (_callers.find(address) == _callers.end())
    {
        auto &caller = _callers[address];
        /* Fill in the caller info structure */
        caller.function_address = address;
        caller.caller_address = build_cdecl_caller(address, argc);
        caller.argc = argc;
        caller.cc = enCallConvention::ECC_CDECL;
    }
    // TODO: Optimize: store last args and send new only if there are diffs.
    send_external_caller_arguments(_callers[address],
                                   reinterpret_cast<uintptr_t>(&argc) + 4);

    return call_external_function(_callers[address]);
}

uintptr_t ExternalProcess::call_stdcall_function(uintptr_t address, size_t argc,
                                                 ...)
{
    if (_callers.find(address) == _callers.end())
    {
        auto &caller = _callers[address];
        /* Fill in the caller info structure */
        caller.function_address = address;
        caller.caller_address = build_stdcall_caller(address, argc);
        caller.argc = argc;
        caller.cc = enCallConvention::ECC_STDCALL;
    }
    // TODO: Optimize: store last args and send new only if there are diffs.
    send_external_caller_arguments(_callers[address],
                                   reinterpret_cast<uintptr_t>(&argc) + 4);

    return call_external_function(_callers[address]);
}

uintptr_t ExternalProcess::call_thiscall_function(uintptr_t address,
                                                  uintptr_t this_ptr,
                                                  size_t argc, ...)
{
    if (_callers.find(address) == _callers.end())
    {
        auto &caller = _callers[address];
        /* Fill in the caller info structure */
        caller.function_address = address;
        caller.caller_address = build_thiscall_caller(address, argc);
        caller.argc = argc;
        caller.cc = enCallConvention::ECC_THISCALL;
    }
    // TODO: Optimize: store last args and send new only if there are diffs.
    send_external_caller_arguments(_callers[address],
                                   reinterpret_cast<uintptr_t>(&argc) + 4);
    send_thiscall_this_ptr(_callers[address], this_ptr);
    return call_external_function(_callers[address]);
}

void ExternalProcess::inject_code(uintptr_t address, const uint8_t *bytes,
                                  size_t bytes_size,
                                  size_t overwrite_bytes_size,
                                  enInjectionType it)
{
    // TODO: Implement call-based injector.
    // TODO: Support injecting and uninjecting multiple code-injections in the
    //       same address.

    /* If colde has not already injected on @address address */
    if (_injected_code.find(address) == _injected_code.end())
    {
        uintptr_t result = 0;
        switch (it)
        {
        case enInjectionType::EIT_JMP:
            result = inject_code_using_jmp(address, bytes, bytes_size,
                                           overwrite_bytes_size);
            break;
        case enInjectionType::EIT_PUSHRET:
            result = inject_code_using_push_ret(address, bytes, bytes_size,
                                                overwrite_bytes_size);
            break;
        }
        if (result)
        {
            InjectedCodeInfo ici;
            ici.injected_bytes_number = bytes_size;
            ici.overwritten_bytes_number = overwrite_bytes_size;
            ici.allocated_buffer = result;
            _injected_code[address] = ici;
        }
    }
}

void ExternalProcess::uninject_code(uintptr_t address)
{
    auto ici = _injected_code.find(address);
    if (ici != _injected_code.end())
    {
        /* Set vp */
        set_virtual_protect(address, ici->second.overwritten_bytes_number,
                            enVirtualProtect::READ_WRITE_EXECUTE);

        /* Create buffer to store original bytes */
        uint8_t *original_bytes =
            new uint8_t[ici->second.overwritten_bytes_number];

        /* Read original bytes to this buffer */
        read_buf(ici->second.allocated_buffer +
                     ici->second.injected_bytes_number,
                 ici->second.overwritten_bytes_number, original_bytes);

        /* Restore original bytes */
        write_buf(ici->first, ici->second.overwritten_bytes_number,
                  original_bytes);

        /* Restore vp */
        restore_virtual_protect(address);

        delete[] original_bytes;
        free(ici->second.allocated_buffer);
        _injected_code.erase(address);
    }
}

void ExternalProcess::patch(uintptr_t address, const uint8_t *bytes,
                            size_t size)
{
    // TODO: Store patched bytes to be able to unpatch later.
    /* Set vp */
    set_virtual_protect(address, size, enVirtualProtect::READ_WRITE_EXECUTE);

    /* Write bytes */
    write_buf(address, size, bytes);

    /* Restore vp */
    restore_virtual_protect(address);
}

uintptr_t ExternalProcess::find_signature(uintptr_t address, size_t size,
                                          const uint8_t *signature,
                                          const char *mask) const
{
    if (!signature)
    {
        return 0;
    }
    if (!mask)
    {
        return 0;
    }
    if (strlen(mask) > size)
    {
        return 0;
    }
    uint8_t *buffer = new uint8_t[size];
    read_buf(address, size, buffer);
    uintptr_t result = 0;
    for (size_t i = 0; i <= size - strlen(mask); i++)
    {
        uintptr_t mask_offset = 0;
        while (mask[mask_offset])
        {
            if (mask[mask_offset] == 'x' &&
                buffer[i + mask_offset] != signature[mask_offset])
            {
                mask_offset = 0;
                break;
            }
            ++mask_offset;
        }
        if (mask_offset != 0)
        {
            result = address + i;
            break;
        }
    }
    delete[] buffer;
    return result;
}

uint32_t ExternalProcess::get_process_id_by_process_name(
    const char *process_name) const
{
    uint32_t result = 0;
    HANDLE snapshot_handle =
        WINAPI_CALL(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (snapshot_handle != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 process_entry;
        process_entry.dwSize = sizeof(process_entry);
        if (Process32First(snapshot_handle, &process_entry))
        {
            do
            {
                if (!_strcmpi(process_entry.szExeFile, process_name))
                {
                    result = process_entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot_handle, &process_entry));
        }
    }
    WINAPI_CALL(CloseHandle(snapshot_handle));
    return result;
}

void ExternalProcess::provide_debug_access(void)
{
    WINAPI_CALL(OpenProcessToken(static_cast<HANDLE>(_handle),
                                 TOKEN_ADJUST_PRIVILEGES, &_dbg_handle));

    TOKEN_PRIVILEGES tp;
    LUID luid;

    WINAPI_CALL(LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid));

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    WINAPI_CALL(AdjustTokenPrivileges(_dbg_handle, FALSE, &tp,
                                      sizeof(TOKEN_PRIVILEGES),
                                      (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL));
}

uintptr_t ExternalProcess::build_cdecl_caller(uintptr_t address, size_t argc)
{
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * argc + /* (push-instruction size + argument size) * argc */
        1 + 4 +          /* call-instruction size + address size */
        3 +              /* restore-stack-instructions size */
        1;               /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space */
    uintptr_t caller_address = alloc(caller_size);

    /* Create local buffer to be sent in remote process */
    uint8_t *local_caller_buffer = new uint8_t[caller_size];

    /* Write push-args instructions in the caller. */
    for (size_t i = 0; i < argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uintptr_t *>(local_caller_buffer + i * 5 + 1) = 0;
    }

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[argc * 5] = 0xe8; /* call */

    /* Calculate and write an address for the near call instruction */
    *reinterpret_cast<uintptr_t *>(local_caller_buffer + argc * 5 + 1) =
        address - (caller_address + argc * 5) - 5;

    /* Write restore-stack-instructions to the caller bytes */
    local_caller_buffer[argc * 5 + 5] = 0x83;     /* add */
    local_caller_buffer[argc * 5 + 6] = 0xc4;     /* esp */
    local_caller_buffer[argc * 5 + 7] = 4 * argc; /* args size */

    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 8] = 0xc3; /* ret */

    /* Write caller bytes to the remote process's memory */
    write_buf(caller_address, caller_size, local_caller_buffer);

    delete[] local_caller_buffer;

    return caller_address;
}

uintptr_t ExternalProcess::build_stdcall_caller(uintptr_t address, size_t argc)
{
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * argc + /* (push-instruction size + argument size) * argc */
        1 + 4 +          /* call-instruction size + address size */
        1;               /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space */
    uintptr_t caller_address = alloc(caller_size);

    /* Create local buffer to be sent in remote process */
    uint8_t *local_caller_buffer = new uint8_t[caller_size];

    /* Write push-args instructions in the caller. */
    for (size_t i = 0; i < argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uintptr_t *>(local_caller_buffer + i * 5 + 1) = 0;
    }

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[argc * 5] = 0xe8; /* call */

    /* Calculate and write an address for the near call instruction */
    *reinterpret_cast<uintptr_t *>(local_caller_buffer + argc * 5 + 1) =
        address - (caller_address + argc * 5) - 5;

    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 5] = 0xc3; /* ret */

    /* Write caller bytes to the remote process's memory */
    write_buf(caller_address, caller_size, local_caller_buffer);

    delete[] local_caller_buffer;

    return caller_address;
}

uintptr_t ExternalProcess::build_thiscall_caller(uintptr_t address, size_t argc)
{
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * argc + /* (push-instruction size + argument size) * argc */
        5 +              /* mov _this to ecx */
        1 + 4 +          /* call-instruction size + address size */
        1;               /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space */
    uintptr_t caller_address = alloc(caller_size);

    /* Create local buffer to be sent in remote process */
    uint8_t *local_caller_buffer = new uint8_t[caller_size];

    /* Write push-args instructions in the caller. */
    for (size_t i = 0; i < argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uintptr_t *>(local_caller_buffer + i * 5 + 1) = 0;
    }

    /* Write [mov ecx, _this] to the caller bytes */
    local_caller_buffer[argc * 5] = 0xB9; /* mov ecx, */
    *reinterpret_cast<uintptr_t *>(local_caller_buffer + argc * 5 + 1) = 0;

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 5] = 0xe8; /* call */

    /* Calculate and write an address for the near call instruction */
    *reinterpret_cast<uintptr_t *>(local_caller_buffer + argc * 5 + 5 + 1) =
        address - (caller_address + argc * 5 + 5) - 5;

    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 5 + 5] = 0xc3; /* ret */

    /* Write caller bytes to the remote process's memory */
    write_buf(caller_address, caller_size, local_caller_buffer);

    delete[] local_caller_buffer;

    return caller_address;
}

void ExternalProcess::send_external_caller_arguments(ExternalCaller const &ec,
                                                     uintptr_t args, ...)
{
    // for (uint32_t i = 0; i < ec.argc; i++)
    //{
    //    write<uint32_t>(ec.caller_address + i * 5 + 1,
    //                    *(((uint32_t *)args) + ec.argc - i - 1));
    //}

    /* Local buffer to build code for pushing arguments onto stack */
    uint8_t *push_args_block = new uint8_t[ec.argc * 5];

    /* Write push-args instructions in the local buffer */
    for (size_t i = 0; i < ec.argc; i++)
    {
        push_args_block[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uintptr_t *>(push_args_block + i * 5 + 1) =
            *(((uintptr_t *)args) + ec.argc - i - 1);
    }

    /* Write the local buffer in caller @ec in the external process */
    write_buf(ec.caller_address, ec.argc * 5, push_args_block);

    delete[] push_args_block;
}

void ExternalProcess::send_thiscall_this_ptr(const ExternalCaller &ec,
                                             uintptr_t this_ptr)
{
    if (ec.cc == enCallConvention::ECC_THISCALL)
    {
        write<uintptr_t>(ec.caller_address + ec.argc * 5 + 1, this_ptr);
    }
    return;
}

uintptr_t ExternalProcess::call_external_function(
    const ExternalProcess::ExternalCaller &ec) const
{
    /* Create a thread in the remote process */
    HANDLE thread_handle = WINAPI_CALL(CreateRemoteThread(
        static_cast<HANDLE>(_handle), NULL, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(ec.caller_address), NULL, 0,
        NULL));

    /* Wait for a return from the function */
    while (WaitForSingleObject(thread_handle, 0) != 0)
    {
    }

    uintptr_t result = 0;
    /* Get the returned value */
    WINAPI_CALL(
        GetExitCodeThread(thread_handle, reinterpret_cast<LPDWORD>(&result)));

    WINAPI_CALL(CloseHandle(thread_handle));
    return reinterpret_cast<uintptr_t>(result);
}

uintptr_t ExternalProcess::inject_code_using_jmp(uintptr_t address,
                                                 const uint8_t *bytes,
                                                 size_t size,
                                                 size_t overwrite_bytes_size)
{
    if (overwrite_bytes_size < 5) /* should be at least 5 bytes for jmp */
    {
        return 0;
    }
    /* Set vp */
    set_virtual_protect(address, overwrite_bytes_size,
                        enVirtualProtect::READ_WRITE_EXECUTE);

    /* Store original bytes to be overwritten */
    uint8_t *original_bytes = new uint8_t[overwrite_bytes_size];
    read_buf(address, overwrite_bytes_size, original_bytes);

    /* Calculate whole remote process buffer size */
    size_t allocated_buf_size = size + overwrite_bytes_size + 5;

    /* Allocate memory for remote buffer */
    uintptr_t allocated_buf = alloc(allocated_buf_size);

    /* Create local buffer to be sent in remote process' buf */
    uint8_t *local_buf = new uint8_t[allocated_buf_size];

    /* Fill local buffer with data */
    /* Put injected bytes in local buffer */
    memcpy(local_buf, bytes, size);

    /* Put original bytes in local buffer */
    memcpy(local_buf + size, original_bytes, overwrite_bytes_size);

    /* Put jmp instruction in local buffer */
    local_buf[size + overwrite_bytes_size] = 0xE9;

    /* Put jmp address in local buffer */
    *reinterpret_cast<uintptr_t *>(
        &(local_buf[size + overwrite_bytes_size + 1])) =
        (address + overwrite_bytes_size) -
        (allocated_buf + size + overwrite_bytes_size) - 5;

    /* Write local buffer in remote process' allocated buffer */
    write_buf(allocated_buf, allocated_buf_size, local_buf);

    /* Overwrite original bytes */
    /* Don't need local_buf anymore, so it can be used here */
    size_t nops_number = overwrite_bytes_size - 5;
    if (nops_number > 0)
    {
        memset(local_buf, 0x90, nops_number); /* Put nops in local buffer */
    }
    local_buf[nops_number] = 0xE9; /* Put jmp instruction in local buffer */

    /* Put jmp address in local buffer */
    *reinterpret_cast<uintptr_t *>(&(local_buf[nops_number + 1])) =
        allocated_buf - (address + nops_number) - 5;

    /* Write local buffer in remote process */
    write_buf(address, overwrite_bytes_size, local_buf);

    /* Restore vp */
    restore_virtual_protect(address);

    delete[] original_bytes;
    delete[] local_buf;
    return allocated_buf;
}

uintptr_t ExternalProcess::inject_code_using_push_ret(
    uintptr_t address, const uint8_t *bytes, size_t bytes_size,
    size_t overwrite_bytes_size)
{
    if (overwrite_bytes_size < 6) /* should be at least 6 bytes for push-ret */
    {
        return 0;
    }
    /* Set vp */
    set_virtual_protect(address, overwrite_bytes_size,
                        enVirtualProtect::READ_WRITE_EXECUTE);

    /* Store original bytes to be overwritten */
    uint8_t *original_bytes = new uint8_t[overwrite_bytes_size];
    read_buf(address, overwrite_bytes_size, original_bytes);

    /* Calculate whole remote process buffer size */
    size_t allocated_buf_size = bytes_size + overwrite_bytes_size + 5;

    /* Allocate memory for remote buffer */
    uintptr_t allocated_buf = alloc(allocated_buf_size);

    /* Create local buffer to be sent in remote process' buf */
    uint8_t *local_buf = new uint8_t[allocated_buf_size];

    /* Fill local buffer with data */
    /* Put injected bytes in local buffer */
    memcpy(local_buf, bytes, bytes_size);

    /* Put original bytes in local buffer */
    memcpy(local_buf + bytes_size, original_bytes, overwrite_bytes_size);

    /* Put jmp instruction in local buffer */
    local_buf[bytes_size + overwrite_bytes_size] = 0xE9;

    /* Put jmp address in local buffer */
    *reinterpret_cast<uintptr_t *>(
        &(local_buf[bytes_size + overwrite_bytes_size + 1])) =
        (address + overwrite_bytes_size) -
        (allocated_buf + bytes_size + overwrite_bytes_size) - 5;

    /* Write local buffer in remote process' allocated buffer */
    write_buf(allocated_buf, allocated_buf_size, local_buf);

    /* Overwrite original bytes */
    /* Don't need local_buf anymore, so it can be used here */
    size_t nops_number = overwrite_bytes_size - 6;
    if (nops_number > 0)
    {
        memset(local_buf, 0x90, nops_number); /* Put nops in local buffer */
    }
    local_buf[nops_number] = 0x68; /* Put push instruction in local buffer */
    /* Put return address in local buffer */
    *reinterpret_cast<uintptr_t *>(&(local_buf[nops_number + 1])) =
        allocated_buf;
    /* Put ret instruction in local buffer */
    local_buf[nops_number + 5] = 0xC3;

    /* Write local buffer in remote process */
    write_buf(address, overwrite_bytes_size, local_buf);

    /* Restore vp */
    restore_virtual_protect(address);

    delete[] original_bytes;
    delete[] local_buf;
    return allocated_buf;
}

/**-----------------------------------------------------------------------------
; @file external_process.cpp
;
; @brief
;   The file contains class members and methods specification of ExternalProcess
;   class.
;
; @author ep1h
;-----------------------------------------------------------------------------*/
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

/**-----------------------------------------------------------------------------
; @ExternalProcess
;
; @brief
;   Constructor. This constructor takes a process id @process_id, gets the
;   process handle using OpenProcess method and assigns it to @_handle field.
;-----------------------------------------------------------------------------*/
ExternalProcess::ExternalProcess(uint32_t process_id) : _process_id(process_id)
{
    _handle = WINAPI_CALL(OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id));
}

/**-----------------------------------------------------------------------------
; @ExternalProcess
;
; @brief
;   Constructor. This constructor takes a process name @process_name, gets the
;   process id using @get_process_id_by_process_name method and delegates object
;   creation to the constructor that takes a process id.
;-----------------------------------------------------------------------------*/
ExternalProcess::ExternalProcess(const char *process_name)
    : ExternalProcess(get_process_id_by_process_name(process_name))
{
}

/**-----------------------------------------------------------------------------
; @~ExternalProcess
;
; @brief
;   Destructor. Removes all created external callers and injected code from
;   remote process memory. Frees all memory allocated in remote process. Closes
;   process handle.
;-----------------------------------------------------------------------------*/
ExternalProcess::~ExternalProcess(void)
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

/**-----------------------------------------------------------------------------
; @read_buf
;
; @brief
;   Reads @size bytes at @address address of the external process.
;   Writes the result to @out_result.
;-----------------------------------------------------------------------------*/
void ExternalProcess::read_buf(uint32_t address, uint32_t size,
                               void *out_result) const
{
    // ReadProcessMemory(static_cast<HANDLE>(_handle),
    //                   reinterpret_cast<LPCVOID>(address), out_result, size,
    //                   0);
    NtReadVirtualMemory(static_cast<HANDLE>(_handle),
                        reinterpret_cast<PVOID>(address), out_result, size, 0);
}

/**-----------------------------------------------------------------------------
; @write_buf
;
; @brief
;   Writes @size bytes from @data buffer to the external process at @address
;   address.
;-----------------------------------------------------------------------------*/
void ExternalProcess::write_buf(uint32_t address, uint32_t size,
                                const void *data) const
{
    // WriteProcessMemory(static_cast<HANDLE>(_handle),
    //                    reinterpret_cast<LPVOID>(address), data, size, 0);

    NtWriteVirtualMemory(static_cast<HANDLE>(_handle),
                         reinterpret_cast<PVOID>(address),
                         const_cast<PVOID>(data), size, 0);
}

/**-----------------------------------------------------------------------------
; @alloc
;
; @brief
;   Allocates @size bytes in the external process. If allocation is successful,
;   adds an entry to the @_allocated_memory map, where the key is the address
;   where the memory was allocated, the value is the number of allocated bytes.
;
; @return
;   Address of the allocated memory if success.
;   NULL if fail.
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::alloc(const uint32_t size)
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
        _allocated_memory[reinterpret_cast<uint32_t>(address)] = size;
    }
    return reinterpret_cast<uint32_t>(address);
}

/**-----------------------------------------------------------------------------
; @free
;
; @brief
;   Frees allocated region located at @address address in the external process.
;   Removes an entry from the @_allocated_memory map, where the key is @address.
;-----------------------------------------------------------------------------*/
void ExternalProcess::free(uint32_t address)
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

/**-----------------------------------------------------------------------------
; @set_virtual_protect
;
; @brief
;   Changes the protection on a region of committed pages in the virtual address
;   space of the calling process.
;
; @param address    Region address.
; @param size       Region size.
; @param type       Protection type.
;-----------------------------------------------------------------------------*/
void ExternalProcess::set_virtual_protect(uint32_t address, uint32_t size,
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

/**-----------------------------------------------------------------------------
; @restore_virtual_protect
;
; @brief
;   Restores the original protection on a region of committed pages in the
;   virtual address space of the calling process.
;
; @param address    Region address.
;-----------------------------------------------------------------------------*/
void ExternalProcess::restore_virtual_protect(uint32_t address)
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

/**-----------------------------------------------------------------------------
; @get_module_address
;
; @brief
;   Return address of module named @module_name (if exists).
;   Otherwise returns zero.
;
; @param module_name    Module name.
;
; @return
;   Module address if success.
;   0 if failire.
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::get_module_address(const char *module_name)
{
    // TODO: Store modules addresses in map.
    uint32_t result = 0;
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
                        reinterpret_cast<uint32_t>(module_entry.modBaseAddr);
                    break;
                }
            } while (Module32Next(snapshot_handle, &module_entry));
        }
    }
    WINAPI_CALL(CloseHandle(snapshot_handle));
    return result;
}

/**-----------------------------------------------------------------------------
; @call_cdecl_function
;
; @brief
;   Allocates a buffer with execution rights in the remote process. Writes to
;   this buffer a set of x86 instructions that call a function at @address with
;   @args arguments using 'cdecl' call convention. Creates a thread that
;   executes the code in this buffer. Waits for this thread to finish executing.
;   Returns a value returned by the function at the @address address (or value
;   of EAX register for void functions).
;
; @param address    An address of the function to be called.
; @param argc       A number of arguments that the function takes.
; @param args       Function arguments (if any).
;
; @return
;   A value returned by the called function (or a value of EAX register for
;   functions of 'void' type).
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::call_cdecl_function(uint32_t address, uint32_t argc,
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
                                   reinterpret_cast<uint32_t>(&argc) + 4);

    return call_external_function(_callers[address]);
}

/**-----------------------------------------------------------------------------
; @call_stdcall_function
;
; @brief
;   Allocates a buffer with execution rights in the remote process. Writes to
;   this buffer a set of x86 instructions that call a function at @address with
;   @args arguments using 'stdcall' call convention. Creates a thread that
;   executes the code in this buffer. Waits for this thread to finish executing.
;   Returns a value returned by the function at the @address address (or value
;   of EAX register for void functions).
;
; @param address    An address of the function to be called.
; @param argc       A number of arguments that the function takes.
; @param args       Function arguments (if any).
;
; @return
;   A value returned by the called function (or a value of EAX register for
;   functions of 'void' type).
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::call_stdcall_function(uint32_t address, uint32_t argc,
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
                                   reinterpret_cast<uint32_t>(&argc) + 4);

    return call_external_function(_callers[address]);
}

/**-----------------------------------------------------------------------------
; @call_thiscall_function
;
; @brief
;   Allocates a buffer with execution rights in the remote process. Writes to
;   this buffer a set of x86 instructions that call a function at @address with
;   @args arguments using 'thiscall' call convention. Creates a thread that
;   executes the code in this buffer. Waits for this thread to finish executing.
;   Returns a value returned by the function at the @address address (or value
;   of EAX register for void functions).
;
; @param address    An address of the function to be called.
; @param this_ptr   A pointer to an object for which the @ec caller will call
;                   method at @ec.function_address address.
; @param argc       A number of arguments that the function takes.
; @param args       Function arguments (if any).
;
; @return
;   A value returned by the called function (or a value of EAX register for
;   functions of 'void' type).
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::call_thiscall_function(uint32_t address,
                                                 uint32_t this_ptr,
                                                 uint32_t argc, ...)
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
                                   reinterpret_cast<uint32_t>(&argc) + 4);
    send_thiscall_this_ptr(_callers[address], this_ptr);
    return call_external_function(_callers[address]);
}

/**-----------------------------------------------------------------------------
; @inject_code
;
; @brief
;   Injects @bytes_size bytes of code into the external process at @address
;   address.
;
; @param address                An address where code should be injected.
; @param bytes                  Code bytes to be injected at @address address.
; @param bytes_size             Number of the bytes to be injected.
; @param overwrite_bytes_size   Number of original bytes to be overwritten and
;                               executed after injected code.
; @param it                     Type of transition to the injected code.
;-----------------------------------------------------------------------------*/
void ExternalProcess::inject_code(uint32_t address, const uint8_t *bytes,
                                  uint32_t bytes_size,
                                  uint32_t overwrite_bytes_size,
                                  enInjectionType it)
{
    // TODO: Implement call-based injector.
    // TODO: Support injecting and uninjecting multiple code-injections in the
    //       same address.

    /* If colde has not already injected on @address address */
    if (_injected_code.find(address) == _injected_code.end())
    {
        uint32_t result = 0;
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

/**-----------------------------------------------------------------------------
; @uninject_code
;
; @brief
;   Removes code injected at @address address (if any), Restores the original
;   bytes overwritten by the injector.
;
; @param address    An address where the code was injected.
;-----------------------------------------------------------------------------*/
void ExternalProcess::uninject_code(uint32_t address)
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

/**-----------------------------------------------------------------------------
; @find_signature
;
; @brief
;   Searches for a sequence of bytes in the memory area of the external process.
;
; @param address    The address from which to start the search.
; @param size       The size of the memory area in which to search for the
;                   signature.
; @param signature  The byte sequence to be found.
; @param mask       The mask to search by. The character 'x' is a complete
;                   match. Characters other than 'x' ignore the corresponding
;                   byte.
;
; @return
;   The address of the first occurrence of @signature by mask @mask in memory
;   area [@address ... @address + @size), or zero on unsuccessful search.
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::find_signature(uint32_t address, uint32_t size,
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
    uint32_t result = 0;
    for (uint32_t i = 0; i <= size - strlen(mask); i++)
    {
        uint32_t mask_offset = 0;
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

/**-----------------------------------------------------------------------------
; @get_process_id_by_process_name
;
; @brief
;   Return process id of process named @process_name (if one is running).
;   Otherwise returns zero.
;
; @param process_name   Process name.
;
; @return
;   Process id if success.
;   0 if failire.
;-----------------------------------------------------------------------------*/
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

// TODO: Write description, test
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

/**-----------------------------------------------------------------------------
; @build_cdecl_caller
;
; @brief
;   Creates a buffer with execution rights in the external process. Writes to
;   this buffer a set of i686 instructions that:
;       - push arguments in the amount of @argc onto the stack
;       - call function at @address using 'cdecl' call convention
;       - restore stack
;       - return
;
;   cdecl-function caller structure:
;     BYTES         INSTRUCTION     SIZE    COMMENT
;     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the last arg. value
;     ...
;     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 2-nd arg. value
;     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 1-st arg. value
;     E8 XXXXXXXX   call XXXXXXXX   5       XXXXXXXX = function address
;     83 C4 XX      add esp, XX     3       Restore stack. XX = 4 * @argc
;     C3            ret             1       Return (terminate thread)
;
; @param address    An address of a function to be called.
; @param argc       A number of arguments that the function takes.
;
; @return           Address where the caller is placed in the external process.
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::build_cdecl_caller(uint32_t address, uint32_t argc)
{
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * argc + /* (push-instruction size + argument size) * argc */
        1 + 4 +          /* call-instruction size + address size */
        3 +              /* restore-stack-instructions size */
        1;               /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space */
    uint32_t caller_address = alloc(caller_size);

    /* Create local buffer to be sent in remote process */
    uint8_t *local_caller_buffer = new uint8_t[caller_size];

    /* Write push-args instructions in the caller. */
    for (uint32_t i = 0; i < argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uint32_t *>(local_caller_buffer + i * 5 + 1) = 0;
    }

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[argc * 5] = 0xe8; /* call */

    /* Calculate and write an address for the near call instruction */
    *reinterpret_cast<uint32_t *>(local_caller_buffer + argc * 5 + 1) =
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

/**-----------------------------------------------------------------------------
; @build_stdcall_caller
;
; @brief
;   Creates a buffer with execution rights in the external process. Writes to
;   this buffer a set of i686 instructions that:
;       - push arguments in the amount of @argc onto the stack
;       - call function at @address using 'stdcall' call convention
;       - return
;
;   stdcall-function caller structure:
;     BYTES         INSTRUCTION     SIZE    COMMENT
;     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the last arg. value
;     ...
;     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 2-nd arg. value
;     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 1-st arg. value
;     E8 XXXXXXXX   call XXXXXXXX   5       XXXXXXXX = function address
;     C3            ret             1       Return (terminate the thread)
;
; @param address    An address of the function to be called.
; @param argc       A number of arguments that the function takes.
;
; @return           Address where the caller is placed in the external process.
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::build_stdcall_caller(uint32_t address, uint32_t argc)
{
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * argc + /* (push-instruction size + argument size) * argc */
        1 + 4 +          /* call-instruction size + address size */
        1;               /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space */
    uint32_t caller_address = alloc(caller_size);

    /* Create local buffer to be sent in remote process */
    uint8_t *local_caller_buffer = new uint8_t[caller_size];

    /* Write push-args instructions in the caller. */
    for (uint32_t i = 0; i < argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uint32_t *>(local_caller_buffer + i * 5 + 1) = 0;
    }

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[argc * 5] = 0xe8; /* call */

    /* Calculate and write an address for the near call instruction */
    *reinterpret_cast<uint32_t *>(local_caller_buffer + argc * 5 + 1) =
        address - (caller_address + argc * 5) - 5;

    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 5] = 0xc3; /* ret */

    /* Write caller bytes to the remote process's memory */
    write_buf(caller_address, caller_size, local_caller_buffer);

    delete[] local_caller_buffer;

    return caller_address;
}

/**-----------------------------------------------------------------------------
; @build_thiscall_caller
;
; @brief
;   Creates a buffer with execution rights in the external process. Writes to
;   this buffer a set of i686 instructions that:
;       - push arguments in the amount of @argc onto the stack
;       - put 'this' for which a method at @address is called into EAX register
;       - call class method at @address using 'stdcall' call convention
;       - return
;
;   thiscall-function caller structure:
;     BYTES         INSTRUCTION         SIZE    COMMENT
;     68 XXXXXXXX   push XXXXXXXX       5       XXXXXXXX = the last arg. value
;     ...
;     68 XXXXXXXX   push XXXXXXXX       5       XXXXXXXX = the 2-nd arg. value
;     68 XXXXXXXX   push XXXXXXXX       5       XXXXXXXX = the 1-st arg. value
;     B9 XXXXXXXX   mov ecx, XXXXXXXX   5       XXXXXXXX = 'this'
;     E8 XXXXXXXX   call XXXXXXXX       5       XXXXXXXX = function address
;     C3            ret                 1       Return (terminate the thread)
;
; @param address    An address of the function to be called.
; @param argc       A number of arguments that the function takes.
;
; @return           Address where the caller is placed in the external process.
-----------------------------------------------------------------------------**/
uint32_t ExternalProcess::build_thiscall_caller(uint32_t address, uint32_t argc)
{
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * argc + /* (push-instruction size + argument size) * argc */
        5 +              /* mov _this to ecx */
        1 + 4 +          /* call-instruction size + address size */
        1;               /* ret-instruction size */

    /* Allocate space for the caller in the remote process's address space */
    uint32_t caller_address = alloc(caller_size);

    /* Create local buffer to be sent in remote process */
    uint8_t *local_caller_buffer = new uint8_t[caller_size];

    /* Write push-args instructions in the caller. */
    for (uint32_t i = 0; i < argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uint32_t *>(local_caller_buffer + i * 5 + 1) = 0;
    }

    /* Write [mov ecx, _this] to the caller bytes */
    local_caller_buffer[argc * 5] = 0xB9; /* mov ecx, */
    *reinterpret_cast<uint32_t *>(local_caller_buffer + argc * 5 + 1) = 0;

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 5] = 0xe8; /* call */

    /* Calculate and write an address for the near call instruction */
    *reinterpret_cast<uint32_t *>(local_caller_buffer + argc * 5 + 5 + 1) =
        address - (caller_address + argc * 5 + 5) - 5;

    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[argc * 5 + 5 + 5] = 0xc3; /* ret */

    /* Write caller bytes to the remote process's memory */
    write_buf(caller_address, caller_size, local_caller_buffer);

    delete[] local_caller_buffer;

    return caller_address;
}

/**-----------------------------------------------------------------------------
; @send_external_caller_arguments
;
; @brief
;   Writes arguments @args with which the function should be called.
;   Since instructions for pushing arguments on stack for all implemented
;   callers (cdecl, thiscall, stdcall) are at the very beginning, this function
;   is universal and applicable to callers of all call conventions listed above.
;
; @param ec     A caller to specify arguments for.
; @param args   Arguments.
-----------------------------------------------------------------------------**/
void ExternalProcess::send_external_caller_arguments(ExternalCaller const &ec,
                                                     uint32_t args, ...)
{
    // for (uint32_t i = 0; i < ec.argc; i++)
    //{
    //    write<uint32_t>(ec.caller_address + i * 5 + 1,
    //                    *(((uint32_t *)args) + ec.argc - i - 1));
    //}

    /* Local buffer to build code for pushing arguments onto stack */
    uint8_t *push_args_block = new uint8_t[ec.argc * 5];

    /* Write push-args instructions in the local buffer */
    for (uint32_t i = 0; i < ec.argc; i++)
    {
        push_args_block[i * 5] = 0x68; /* push */
        /* argument */
        *reinterpret_cast<uint32_t *>(push_args_block + i * 5 + 1) =
            *(((uint32_t *)args) + ec.argc - i - 1);
    }

    /* Write the local buffer in caller @ec in the external process */
    write_buf(ec.caller_address, ec.argc * 5, push_args_block);

    delete[] push_args_block;
}

/**-----------------------------------------------------------------------------
; @send_thiscall_this_ptr
;
; @brief
;   Puts @this_ptr in caller @ec. The ffset is calculated as follows:
;       @ec.argc * 5 is a push args instructions size. +1 is a mov ecx
;       instruction size. It must be followed by @this_ptr.
;
; @param ec         A caller to specify class object ptr for.
; @param this_ptr   A pointer to an object for which the @ec caller will call
;                   method at @ec.function_address address.
-----------------------------------------------------------------------------**/
void ExternalProcess::send_thiscall_this_ptr(const ExternalCaller &ec,
                                             uint32_t this_ptr)
{
    if (ec.cc == enCallConvention::ECC_THISCALL)
    {
        write<uint32_t>(ec.caller_address + ec.argc * 5 + 1, this_ptr);
    }
    return;
}

/**-----------------------------------------------------------------------------
; @call_external_function
;
; @brief
;   Creates a thread in remote process. This thread executes the code located in
;   @ec.caller_address buffer. Waits for this thread to finish executing.
;   Returns a value returned by the function at the @ec.function+address address
;   (or a value of EAX register for void functions).
;
; @param e?? Caller whose function is to be called.
;
; @return   A value returned by the called function (or a value of EAX register
;           for functions of type 'void').
-----------------------------------------------------------------------------**/
uint32_t ExternalProcess::call_external_function(
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

    DWORD result = 0;
    /* Get the returned value */
    WINAPI_CALL(GetExitCodeThread(thread_handle, &result));

    WINAPI_CALL(CloseHandle(thread_handle));
    return result;
}

/**-----------------------------------------------------------------------------
; @inject_code_using_jmp
;
; @brief
;   Injects @bytes_size bytes of code from the @bytes argument into remote
;   process at @address address.
;
;   Logic:
;   Allocates a @allocated_buf buffer in a remote process with execute
;   permissions.
;   Writes @bytes injected code to it. Writes an unconditional jump instruction
;   'jmp'to @allocated_buf at @address. This jmp instruction overwrites 5 bytes
;   at @address. These 5 bytes are recovered in the @allocated_buf buffer after
;   the injected code.
;   There are cases where the fifth byte (@address+4) is not the last byte of
;   the instruction. In this case, the number of overwritten bytes must be
;   specified explicitly (argument @overwrite_bytes_size) and all of them will
;   be executed in the @allocated_buf buffer after the injected code.
;   If @overwrite_bytes_size is greater than 5, then to save addressing, the
;   first (@overwrite_bytes_size - 5) bytes at @address will be replaced with
;   nops (0x90).
;   In the example below, there is a situation when it is impossible to write a
;   jmp instruction at @address without explicitly specifying
;   @overwrite_bytes_size.
;
;   Original bytes:
;     ADDRESS                           INSTRUCTION(S)      INSTRUCTION(S) LEN
;     @address + 00                     instruction #1      4
;     @address + 04                     instruction #2      2
;     @address + 06                     instruction #3      4
;
;   After injection with @overwrite_bytes_size = 6:
;     ADDRESS                           INSTRUCTION(S)      INSTRUCTION(S) LEN
;     @address + 00                     nop                 1
;     @address + 01                     jmp @allocated_buf  5
;     @address + 06                     instruction #3      4
;
;   Allocated buffer:
;     ADDRESS                           INSTRUCTION(S)      INSTRUCTION(S) LEN
;     @allocated_buf + 00               injected code       @size
;     @allocated_buf + @size            instruction #1      4
;     @allocated_buf + @size + 4        instruction #2      2
;     @allocated_buf + @size + 4 + 2    jmp @address + 06   5
;
; @param address                An address where code should be injected.
; @param bytes                  Code bytes to be injected at 'address' address.
; @param bytes_size             Number of the bytes to be injected.
; @param overwrite_bytes_size   The number of original bytes to be overwritten
;                               and executed after injected code.
;
; @return allocated buffer address @allocated_buf
-----------------------------------------------------------------------------**/
uint32_t ExternalProcess::inject_code_using_jmp(uint32_t address,
                                                const uint8_t *bytes,
                                                uint32_t size,
                                                uint32_t overwrite_bytes_size)
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
    uint32_t allocated_buf_size = size + overwrite_bytes_size + 5;

    /* Allocate memory for remote buffer */
    uint32_t allocated_buf = alloc(allocated_buf_size);

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
    *reinterpret_cast<uint32_t *>(
        &(local_buf[size + overwrite_bytes_size + 1])) =
        (address + overwrite_bytes_size) -
        (allocated_buf + size + overwrite_bytes_size) - 5;

    /* Write local buffer in remote process' allocated buffer */
    write_buf(allocated_buf, allocated_buf_size, local_buf);

    /* Overwrite original bytes */
    /* Don't need local_buf anymore, so it can be used here */
    uint32_t nops_number = overwrite_bytes_size - 5;
    if (nops_number > 0)
    {
        memset(local_buf, 0x90, nops_number); /* Put nops in local buffer */
    }
    local_buf[nops_number] = 0xE9; /* Put jmp instruction in local buffer */

    /* Put jmp address in local buffer */
    *reinterpret_cast<uint32_t *>(&(local_buf[nops_number + 1])) =
        allocated_buf - (address + nops_number) - 5;

    /* Write local buffer in remote process */
    write_buf(address, overwrite_bytes_size, local_buf);

    /* Restore vp */
    restore_virtual_protect(address);

    delete[] original_bytes;
    delete[] local_buf;
    return allocated_buf;
}

/**-----------------------------------------------------------------------------
; @inject_code_using_push_ret
;
; @brief
;   Injects @bytes_size bytes of code from the @bytes argument into remote
;   process at @address address.
;
; @param address                An address where code should be injected.
; @param bytes                  Code bytes to be injected at 'address' address.
; @param bytes_size             Number of the bytes to be injected.
; @param overwrite_bytes_size   The number of original bytes to be overwritten
;                               and executed after injected code.
;
; @return allocated buffer address @allocated_buf
-----------------------------------------------------------------------------**/
uint32_t ExternalProcess::inject_code_using_push_ret(
    uint32_t address, const uint8_t *bytes, uint32_t bytes_size,
    uint32_t overwrite_bytes_size)
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
    uint32_t allocated_buf_size = bytes_size + overwrite_bytes_size + 5;

    /* Allocate memory for remote buffer */
    uint32_t allocated_buf = alloc(allocated_buf_size);

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
    *reinterpret_cast<uint32_t *>(
        &(local_buf[bytes_size + overwrite_bytes_size + 1])) =
        (address + overwrite_bytes_size) -
        (allocated_buf + bytes_size + overwrite_bytes_size) - 5;

    /* Write local buffer in remote process' allocated buffer */
    write_buf(allocated_buf, allocated_buf_size, local_buf);

    /* Overwrite original bytes */
    /* Don't need local_buf anymore, so it can be used here */
    uint32_t nops_number = overwrite_bytes_size - 6;
    if (nops_number > 0)
    {
        memset(local_buf, 0x90, nops_number); /* Put nops in local buffer */
    }
    local_buf[nops_number] = 0x68; /* Put push instruction in local buffer */
    /* Put return address in local buffer */
    *reinterpret_cast<uint32_t *>(&(local_buf[nops_number + 1])) =
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

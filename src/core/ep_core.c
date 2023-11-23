/**
 * @file ep_core.c
 * @brief Implementation of the ep_core component.
 */
#include <windows.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include "ep_core.h"

#define LOCAL_CALLER_BUF_SIZE 128

typedef struct EPCProcess
{
    HANDLE handle;
    uint32_t process_id;
} EPCProcess;

typedef struct EpCoreFunctionCaller
{
    const EPCProcess *process;
    uintptr_t function_address;
    EPCCallConvention call_convention;
    size_t argc;
    uintptr_t caller_address;
    uintptr_t remote_process_args_ptr;
} EpCoreFunctionCaller;

typedef NTSTATUS(NTAPI *NtReadVirtualMemory_t)(
    IN HANDLE ProcessHandle, IN PVOID BaseAddress, OUT PVOID buffer,
    IN ULONG NumberOfBytesRead, OUT PULONG NumberOfBytesReaded OPTIONAL);

typedef NTSTATUS(NTAPI *NtWriteVirtualMemory_t)(
    IN HANDLE ProcessHandle, IN PVOID BaseAddress, IN PVOID buffer,
    IN ULONG NumberOfBytesToWrite, OUT PULONG NumberOfBytesWritten OPTIONAL);

typedef NTSTATUS(NTAPI *NtAllocateVirtualMemory_t)(
    IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress, IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize, IN ULONG AllocationType, IN ULONG Protect);

typedef NTSTATUS(NTAPI *NtFreeVirtualMemory_t)(IN HANDLE ProcessHandle,
                                               IN OUT PVOID *BaseAddress,
                                               IN OUT PSIZE_T RegionSize,
                                               IN ULONG FreeType);

typedef NTSTATUS(NTAPI *NtProtectVirtualMemory_t)(
    IN HANDLE ProcessHandle, IN OUT PVOID *BaseAddress,
    IN OUT PULONG NumberOfBytesToProtect, IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection);

/**
 * @brief Converts a Windows memory protection type values to the component's
 * memory protection type values.
 * @param[in] protect The Windows memory protection type value.
 * @return The component's memory protection type value.
 */
static EPCVirtualProtect convert_win_protect_to_ep_protect(DWORD protect);

/**
 * @brief Converts the component's memory protection type values to Windows
 * memory protection type values.
 * @param[in] protect The component's memory protection type value.
 * @return The Windows memory protection type value.
 */
static DWORD convert_ep_protect_to_win_protect(EPCVirtualProtect protect);

/**
 * @brief Allocates and initializes a buffer in a remote process for calling
 * functions using 'cdecl' call convention.
 *
 * Creates a buffer with execution rights in the external process. Writes to
 * this buffer a set of i686 instructions that:
 *  - push arguments in the amount of @argc onto the stack
 *  - call function at @address using 'cdecl' call convention
 *  - restore stack
 *  - return
 *
 *   cdecl-function caller structure:
 *     BYTES         INSTRUCTION     SIZE    COMMENT
 *     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the last arg. value
 *     ...
 *     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 2-nd arg. value
 *     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 1-st arg. value
 *     E8 XXXXXXXX   call XXXXXXXX   5       XXXXXXXX = function address
 *     83 C4 XX      add esp, XX     3       Restore stack. XX = 4 * @argc
 *     C3            ret             1       Return (terminate thread)
 *
 * @param[in] process The process for which the caller should be created.
 * @param[in] c The function caller object. It's fields must be initialized
 *              before calling this function.
 * @return The address of the caller buffer in the remote process memory.
 * @return 0 if the caller buffer could not be built.
 */
static uintptr_t build_cdecl_caller(const EPCProcess *process,
                                    EpCoreFunctionCaller *c);

/**
 * @brief Allocates and initializes a buffer in a remote process for calling
 *        functions using 'stdcall' call convention.
 *
 * Creates a buffer with execution rights in the external process. Writes to
 * this buffer a set of i686 instructions that:
 *   - push arguments in the amount of @argc onto the stack
 *   - call function at @address using 'stdcall' call convention
 *   - return
 *
 *   stdcall-function caller structure:
 *     BYTES         INSTRUCTION     SIZE    COMMENT
 *     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the last arg. value
 *     ...
 *     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 2-nd arg. value
 *     68 XXXXXXXX   push XXXXXXXX   5       XXXXXXXX = the 1-st arg. value
 *     E8 XXXXXXXX   call XXXXXXXX   5       XXXXXXXX = function address
 *     C3            ret             1       Return (terminate the thread)
 *
 * @param[in] process The process for which the caller should be created.
 * @param c The function caller object. It's fields must be initialized before
 *          calling this function.
 * @return The address of the caller buffer in the remote process memory.
 * @return 0 if the caller buffer could not be built.
 */
static uintptr_t build_stdcall_caller(const EPCProcess *process,
                                      EpCoreFunctionCaller *c);

/**
 * @brief Allocates and initializes a buffer in a remote process for calling
 *        functions using MSVC 'thiscall' call convention.
 *
 * Creates a buffer with execution rights in the external process. Writes
 * to this buffer a set of i686 instructions that:
 *  - push arguments in the amount of @argc onto the stack
 *  - put 'this' for which a method at @address is called into EAX register
 *  - call class method at @address using 'stdcall' call convention
 *  - return
 *
 * thiscall-function caller structure:
 *  BYTES         INSTRUCTION         SIZE    COMMENT
 *  68 XXXXXXXX   push XXXXXXXX       5       XXXXXXXX = the last arg. value
 *  ...
 *  68 XXXXXXXX   push XXXXXXXX       5       XXXXXXXX = the 2-nd arg. value
 *  68 XXXXXXXX   push XXXXXXXX       5       XXXXXXXX = the 1-st arg. value
 *  B9 XXXXXXXX   mov ecx, XXXXXXXX   5       XXXXXXXX = 'this'
 *  E8 XXXXXXXX   call XXXXXXXX       5       XXXXXXXX = function address
 *  C3            ret                 1       Return (terminate the thread)
 *
 * @param[in] process The process for which the caller should be created.
 * @param c The function caller object. It's fields must be initialized before
 *          calling this function.
 * @return The address of the caller buffer in the remote process memory.
 * @return 0 if the caller buffer could not be built.
 */
static uintptr_t build_thiscall_msvc_caller(const EPCProcess *process,
                                            EpCoreFunctionCaller *c);

static EPCVirtualProtect convert_win_protect_to_ep_protect(DWORD protect)
{
    EPCVirtualProtect result = EPCVP_EMPTY;
    if (protect == PAGE_READONLY)
    {
        result = EPCVP_READ;
    }
    else if (protect == PAGE_READWRITE)
    {
        result = EPCVP_READ | EPCVP_WRITE;
    }
    else if (protect == PAGE_EXECUTE_READ)
    {
        result = EPCVP_READ | EPCVP_EXECUTE;
    }
    else if (protect == PAGE_EXECUTE_READWRITE)
    {
        result = EPCVP_READ | EPCVP_WRITE | EPCVP_EXECUTE;
    }
    else
    {
        // TODO: Throw an error.
    }
    return result;
}

static DWORD convert_ep_protect_to_win_protect(EPCVirtualProtect protect)
{
    DWORD result = 0;
    if (protect == EPCVP_READ)
    {
        result = PAGE_READONLY;
    }
    else if (protect == EPCVP_WRITE)
    {
        result = PAGE_READWRITE;
    }
    else if (protect == (EPCVP_READ | EPCVP_WRITE))
    {
        result = PAGE_READWRITE;
    }
    else if (protect == EPCVP_EXECUTE ||
             protect == (EPCVP_READ | EPCVP_EXECUTE))
    {
        result = PAGE_EXECUTE_READ;
    }
    else if (protect == (EPCVP_READ | EPCVP_WRITE | EPCVP_EXECUTE) ||
             protect == (EPCVP_WRITE | EPCVP_EXECUTE))
    {
        result = PAGE_EXECUTE_READWRITE;
    }
    else
    {
        // TODO: Throw an error.
    }
    return result;
}

static uintptr_t build_cdecl_caller(const EPCProcess *process,
                                    EpCoreFunctionCaller *c)
{
    /* local buffer to be write in remote process' memory */
    uint8_t local_caller_buffer[LOCAL_CALLER_BUF_SIZE] = {0};
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * c->argc + /* (push-instruction size + arg size) * argc */
        1 + 4 +             /* call-instruction size + address size */
        3 +                 /* restore-stack-instructions size */
        1;                  /* ret instruction size */
    if (caller_size > LOCAL_CALLER_BUF_SIZE)
    {
        return 0;
    }
    /* Allocate space for the caller in the remote process's address space */
    uintptr_t caller_address = 0;
    if (!epc_alloc(process, caller_size, &caller_address))
    {
        return 0;
    }
    c->process = process;
    /* Write push-args instructions in the caller. */
    for (size_t i = 0; i < c->argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68;                   /* push */
        *(uintptr_t *)(local_caller_buffer + i * 5 + 1) = 0; /* argument */
    }
    /* Write call-instruction to the caller bytes */
    local_caller_buffer[c->argc * 5] = 0xE8; /* call */
    /* Calculate and write an address for the near call instruction */
    *(uintptr_t *)(local_caller_buffer + c->argc * 5 + 1) =
        c->function_address - (caller_address + c->argc * 5) - 5;
    /* Write restore-stack-instructions to the caller bytes */
    // TOOD: Use 0xC4 ret instead.
    local_caller_buffer[c->argc * 5 + 5] = 0x83;        /* add */
    local_caller_buffer[c->argc * 5 + 6] = 0xC4;        /* esp */
    local_caller_buffer[c->argc * 5 + 7] = 4 * c->argc; /* args size */
    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[c->argc * 5 + 8] = 0xC3; /* ret */
    /* Write caller bytes to the remote process's memory */
    if (!epc_write_buf(c->process, caller_address, caller_size,
                       local_caller_buffer))
    {
        return 0;
    }
    return caller_address;
}

static uintptr_t build_stdcall_caller(const EPCProcess *process,
                                      EpCoreFunctionCaller *c)
{
    /* local buffer to be write in remote process' memory */
    uint8_t local_caller_buffer[LOCAL_CALLER_BUF_SIZE] = {0};
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * c->argc + /* (push-instruction size + argument size) * argc */
        1 + 4 +             /* call-instruction size + address size */
        1;                  /* ret-instruction size */
    if (caller_size > LOCAL_CALLER_BUF_SIZE)
    {
        return 0;
    }
    /* Allocate space for the caller in the remote process's address space */
    uintptr_t caller_address = 0;
    if (!epc_alloc(process, caller_size, &caller_address))
    {
        return 0;
    }
    c->process = process;
    /* Write push-args instructions in the caller. */
    for (size_t i = 0; i < c->argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68;                   /* push */
        *(uintptr_t *)(local_caller_buffer + i * 5 + 1) = 0; /* argument */
    }
    /* Write call-instruction to the caller bytes */
    local_caller_buffer[c->argc * 5] = 0xE8; /* call */
    /* Calculate and write an address for the near call instruction */
    *(uintptr_t *)(local_caller_buffer + c->argc * 5 + 1) =
        c->function_address - (caller_address + c->argc * 5) - 5;
    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[c->argc * 5 + 5] = 0xC3; /* ret */
    /* Write caller bytes to the remote process's memory */
    if (!epc_write_buf(c->process, caller_address, caller_size,
                       local_caller_buffer))
    {
        return 0;
    }
    return caller_address;
}

static uintptr_t build_thiscall_msvc_caller(const EPCProcess *process,
                                            EpCoreFunctionCaller *c)
{
    /* local buffer to be write in remote process' memory */
    uint8_t local_caller_buffer[LOCAL_CALLER_BUF_SIZE] = {0};
    /* Calculate caller size */
    size_t caller_size =
        (1 + 4) * c->argc + /* (push-instruction size + argument size) * argc */
        5 +                 /* mov _this to ecx */
        1 + 4 +             /* call-instruction size + address size */
        1;                  /* ret-instruction size */
    if (caller_size > LOCAL_CALLER_BUF_SIZE)
    {
        return 0;
    }
    /* Allocate space for the caller in the remote process's address space */
    uintptr_t caller_address = 0;
    if (!epc_alloc(process, caller_size, &caller_address))
    {
        return 0;
    }
    c->process = process;
    /* Write push-args instructions in the caller. */
    for (size_t i = 0; i < c->argc; i++)
    {
        local_caller_buffer[i * 5] = 0x68;                   /* push */
        *(uintptr_t *)(local_caller_buffer + i * 5 + 1) = 0; /* argument */
    }
    /* Write [mov ecx, _this] to the caller bytes */
    local_caller_buffer[c->argc * 5] = 0xB9; /* mov ecx, */
    *(uintptr_t *)(local_caller_buffer + c->argc * 5 + 1) = 0;

    /* Write call-instruction to the caller bytes */
    local_caller_buffer[c->argc * 5 + 5] = 0xE8; /* call */
    /* Calculate and write an address for the near call instruction */
    *(uintptr_t *)(local_caller_buffer + c->argc * 5 + 5 + 1) =
        c->function_address - (caller_address + c->argc * 5 + 5) - 5;
    /* Write ret-instruction to the caller bytes */
    local_caller_buffer[c->argc * 5 + 5 + 5] = 0xC3; /* ret */
    /* Write caller bytes to the remote process's memory */
    if (!epc_write_buf(c->process, caller_address, caller_size,
                       local_caller_buffer))
    {
        return 0;
    }
    return caller_address;
}

bool epc_create(EPCProcess **out_process)
{
    if (!out_process)
    {
        return false;
    }
    *out_process = (EPCProcess *)malloc(sizeof(**out_process));
    if (!*out_process)
    {
        return false;
    }
    (*out_process)->handle = NULL;
    (*out_process)->process_id = 0;
    return true;
}

bool epc_destroy(EPCProcess *process)
{
    if (!process)
    {
        return false;
    }
    if (process->handle)
    {
        epc_close_process(process);
    }
    free(process);
    return true;
}

bool epc_open_process(EPCProcess *process, unsigned int process_id)
{
    if (!process)
    {
        return false;
    }
    process->handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);
    if (!process->handle)
    {
        return false;
    }
    process->process_id = process_id;
    return true;
}

bool epc_close_process(EPCProcess *process)
{
    if (!process)
    {
        return false;
    }
    if (!CloseHandle(process->handle))
    {
        return false;
    }
    process->handle = NULL;
    return true;
}

bool epc_get_process_id_by_name(const char *process_name,
                                unsigned int *out_process_id)
{
    if (!process_name || !out_process_id)
    {
        return false;
    }
    HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(process_entry);
    if (Process32First(snapshot_handle, &process_entry))
    {
        do
        {
            if (!_strcmpi(process_entry.szExeFile, process_name))
            {
                *out_process_id = process_entry.th32ProcessID;
                if (!CloseHandle(snapshot_handle))
                {
                    return false;
                }
                return true;
            }
        } while (Process32Next(snapshot_handle, &process_entry));
    }
    CloseHandle(snapshot_handle);
    return false;
}

bool epc_read_buf(const EPCProcess *process, uintptr_t address, size_t size,
                  void *out_result)
{
    if (!process || !out_result)
    {
        return false;
    }
    return ReadProcessMemory(process->handle, (LPCVOID)address, out_result,
                             size, 0) != 0;
}

bool epc_read_buf_nt(const EPCProcess *process, uintptr_t address, size_t size,
                     void *out_result)
{
    NtReadVirtualMemory_t NtReadVirtualMemory =
        (NtReadVirtualMemory_t)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                      "NtReadVirtualMemory");
    return NtReadVirtualMemory(process->handle, (PVOID)address, out_result,
                               size, 0) == 0;
}

bool epc_write_buf(const EPCProcess *process, uintptr_t address, size_t size,
                   const void *data)
{
    if (!process || !data)
    {
        return false;
    }
    return WriteProcessMemory(process->handle, (LPVOID)address, data, size,
                              0) != 0;
}
bool epc_write_buf_nt(const EPCProcess *process, uintptr_t address, size_t size,
                      const void *data)
{
    NtWriteVirtualMemory_t NtWriteVirtualMemory =
        (NtWriteVirtualMemory_t)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                       "NtWriteVirtualMemory");
    return NtWriteVirtualMemory(process->handle, (PVOID)address, (PVOID)data,
                                size, 0) == 0;
}

bool epc_alloc(const EPCProcess *process, size_t size, uintptr_t *out_address)
{
    if (!process || !out_address)
    {
        return false;
    }
    *out_address = (uintptr_t)VirtualAllocEx(process->handle, NULL, size,
                                             MEM_COMMIT | MEM_RESERVE,
                                             PAGE_EXECUTE_READWRITE);
    return *out_address != 0;
}
bool epc_alloc_nt(const EPCProcess *process, size_t size,
                  uintptr_t *out_address)
{
    NtAllocateVirtualMemory_t NtAllocateVirtualMemory =
        (NtAllocateVirtualMemory_t)(void *)GetProcAddress(
            LoadLibrary("ntdll.dll"), "NtAllocateVirtualMemory");
    return NtAllocateVirtualMemory(process->handle, (PVOID *)out_address, 0,
                                   (PSIZE_T)&size, MEM_COMMIT | MEM_RESERVE,
                                   PAGE_EXECUTE_READWRITE) == 0;
}
bool epc_free(const EPCProcess *process, uintptr_t address)
{
    if (!process)
    {
        return false;
    }
    return VirtualFreeEx(process->handle, (LPVOID)address, 0, MEM_RELEASE) != 0;
}
bool epc_free_nt(const EPCProcess *process, uintptr_t address)
{
    NtFreeVirtualMemory_t NtFreeVirtualMemory =
        (NtFreeVirtualMemory_t)(void *)GetProcAddress(LoadLibrary("ntdll.dll"),
                                                      "NtFreeVirtualMemory");
    return NtFreeVirtualMemory(process->handle, (PVOID *)&address, 0,
                               MEM_RELEASE) == 0;
}

bool epc_virtual_protect(const EPCProcess *process, uintptr_t address,
                         size_t size, EPCVirtualProtect protect,
                         EPCVirtualProtect *out_old_protect)
{
    if (!process)
    {
        return false;
    }
    if (protect == EPCVP_EMPTY)
    {
        return false;
    }
    if (!out_old_protect)
    {
        return false;
    }
    DWORD new_protect = 0;
    DWORD old_protect = 0;
    new_protect = convert_ep_protect_to_win_protect(protect);
    if (VirtualProtectEx(process->handle, (LPVOID)address, size, new_protect,
                         &old_protect) == 0)
    {
        return false;
    }
    *out_old_protect = convert_win_protect_to_ep_protect(old_protect);
    return true;
}

bool epc_virtual_protect_nt(const EPCProcess *process, uintptr_t address,
                            size_t size, EPCVirtualProtect protect,
                            EPCVirtualProtect *out_old_protect)
{
    NtProtectVirtualMemory_t NtProtectVirtualMemory =
        (NtProtectVirtualMemory_t)(void *)GetProcAddress(
            LoadLibrary("ntdll.dll"), "NtProtectVirtualMemory");
    if (!process)
    {
        return false;
    }
    if (protect == EPCVP_EMPTY)
    {
        return false;
    }
    if (!out_old_protect)
    {
        return false;
    }
    DWORD new_protect = 0;
    DWORD old_protect = 0;
    new_protect = convert_ep_protect_to_win_protect(protect);
    if (NtProtectVirtualMemory(process->handle, (PVOID *)&address,
                               (PULONG)&size, new_protect, &old_protect) != 0)
    {
        return false;
    }
    *out_old_protect = convert_win_protect_to_ep_protect(old_protect);
    return true;
}

bool epc_get_module_address(const EPCProcess *process, const char *module_name,
                            uintptr_t *out_address)
{
    if (!process || !module_name || !out_address)
    {
        return false;
    }
    HANDLE snapshot_handle = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process->process_id);
    if (snapshot_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    MODULEENTRY32 module_entry;
    module_entry.dwSize = sizeof(module_entry);
    if (Module32First(snapshot_handle, &module_entry))
    {
        do
        {
            if (!_strcmpi(module_entry.szModule, module_name))
            {
                *out_address = (uintptr_t)(module_entry.modBaseAddr);
                break;
            }
        } while (Module32Next(snapshot_handle, &module_entry));
    }
    if (!CloseHandle(snapshot_handle))
    {
        return false;
    }
    return true;
}

bool epc_function_caller_build(const EPCProcess *process,
                               uintptr_t function_address,
                               EPCCallConvention call_convention, size_t argc,
                               EpCoreFunctionCaller **out_caller)
{
    if (call_convention <= EPCCC_INVALID || call_convention >= EPCCC_COUNT_)
    {
        return false;
    }
    if (!out_caller)
    {
        return false;
    }
    *out_caller = malloc(sizeof(**out_caller));
    (*out_caller)->process = process;
    (*out_caller)->function_address = function_address;
    (*out_caller)->call_convention = call_convention;
    (*out_caller)->argc = argc;
    switch (call_convention)
    {
    case EPCCC_CDECL: {
        (*out_caller)->caller_address =
            build_cdecl_caller(process, *out_caller);
        break;
    }
    case EPCCC_STDCALL: {
        (*out_caller)->caller_address =
            build_stdcall_caller(process, *out_caller);
        break;
    }
    case EPCCC_THISCALL_MSVC: {
        (*out_caller)->caller_address =
            build_thiscall_msvc_caller(process, *out_caller);
        break;
    }
    default: {
        return false;
    }
    }
    return (*out_caller)->caller_address != 0;
}

bool epc_function_caller_destroy(EpCoreFunctionCaller *caller)
{
    if (!caller)
    {
        return false;
    }
    if (caller->caller_address)
    {
        if (!epc_free(caller->process, caller->caller_address))
        {
            free(caller);
            return false;
        }
    }
    free(caller);
    return true;
}

bool epc_function_caller_send_args(const EpCoreFunctionCaller *caller,
                                   size_t argc, ...)
{
    static uint8_t local_caller_args_buffer[LOCAL_CALLER_BUF_SIZE];
    if (!caller)
    {
        return false;
    }

    if (argc != caller->argc)
    {
        return false;
    }
    if (!caller->caller_address)
    {
        return false;
    }
    uintptr_t argv = (uintptr_t)(&argc) + 4;
    switch (caller->call_convention)
    {
    case EPCCC_CDECL:
    case EPCCC_STDCALL: {
        for (size_t i = 0; i < caller->argc; i++)
        {
            local_caller_args_buffer[i * 5] = 0x68; /* push */
            /* argument */
            *(uintptr_t *)(local_caller_args_buffer + i * 5 + 1) =
                *(((uintptr_t *)argv) + caller->argc - i - 1);
        }
        return epc_write_buf(caller->process, caller->caller_address,
                             caller->argc * 5, local_caller_args_buffer);
    }
    case EPCCC_THISCALL_MSVC: {
        if (caller->argc < 1)
        {
            /* Error. Expected 'this' argument. */
            return false;
        }
        for (size_t i = 1; i < caller->argc; i++)
        {
            local_caller_args_buffer[(i - 1) * 5] = 0x68; /* push */
            /* argument */
            *(uintptr_t *)(local_caller_args_buffer + (i - 1) * 5 + 1) =
                *(((uintptr_t *)argv) + caller->argc - (i - 1) - 1);
        }
        /* MOV ECX, THIS_PTR */
        local_caller_args_buffer[(caller->argc - 1) * 5] = 0xB9;
        *(uintptr_t *)(local_caller_args_buffer + (caller->argc - 1) * 5 + 1) =
            *(uintptr_t *)argv;
        return epc_write_buf(caller->process, caller->caller_address,
                             caller->argc * 5, local_caller_args_buffer);
    }
    default:
        return false;
    }
}

bool epc_function_caller_call(const EpCoreFunctionCaller *caller,
                              uintptr_t *out_result, bool wait_for_return)
{
    // TODO: This function should be able to be called with out_result == NULL.
    if (!caller || !out_result)
    {
        return false;
    }
    /* Create a thread in the remote process */
    HANDLE thread_handle = CreateRemoteThread(
        caller->process->handle, NULL, 0,
        (LPTHREAD_START_ROUTINE)caller->caller_address, NULL, 0, NULL);
    /* Wait for a return from the function */
    while (WaitForSingleObject(caller->process->handle,
                               wait_for_return ? INFINITE : 0) != 0)
    {
    }
    /* Get the returned value */
    if (!GetExitCodeThread(thread_handle, (LPDWORD)(out_result)))
    {
        return false;
    }
    if (!CloseHandle(thread_handle))
    {
        return false;
    }
    return true;
}

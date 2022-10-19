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

using namespace P1ExternalProcess;

/**-----------------------------------------------------------------------------
; @ExternalProcess
;
; @brief
;   Constructor. This constructor assigns @process_handle to @_handle field.
;-----------------------------------------------------------------------------*/
ExternalProcess::ExternalProcess(void *process_handle) : _handle(process_handle)
{
}

/**-----------------------------------------------------------------------------
; @ExternalProcess
;
; @brief
;   Constructor. This constructor takes a process id @process_id, gets the
;   process handle id using OpenProcess method and delegates object creation to
;   the constructor that takes a process handle.
;-----------------------------------------------------------------------------*/
ExternalProcess::ExternalProcess(uint32_t process_id)
    : ExternalProcess(OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id))
{
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
;   Destructor. Frees all memory allocated in remote process. Closes process
;   handle.
;-----------------------------------------------------------------------------*/
ExternalProcess::~ExternalProcess(void)
{
    for (auto i = _allocated_memory.cbegin(), n = i;
         i != _allocated_memory.cend(); i = n)
    {
        ++n;
        free(i->first);
    }
    CloseHandle((HANDLE)_handle);
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
    ReadProcessMemory(static_cast<HANDLE>(_handle),
                      reinterpret_cast<LPCVOID>(address), out_result, size, 0);
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
    WriteProcessMemory(static_cast<HANDLE>(_handle),
                       reinterpret_cast<LPVOID>(address), data, size, 0);
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
    void *address =
        VirtualAllocEx(static_cast<HANDLE>(_handle), NULL, size,
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
        VirtualFreeEx((HANDLE)_handle, reinterpret_cast<void *>(address),
                      result->second, MEM_RELEASE);
        _allocated_memory.erase(address);
    }
}

/**-----------------------------------------------------------------------------
; @call_cdecl_function
;
; @brief
;   Creates a buffer with execution rights at @caller_address address of the
;   external process. Writes to this buffer a set of i686 instructions that:
;       - push arguments in the amount of @argc onto the stack
;       - call function at @address using 'cdecl' call convention
;       - restore stack
;       - return
;   Creates a thread in remote process. This thread executes the code located in
;   @caller_address buffer. Waits for this thread to finish executing.
;   Returns a value returned by the function at the @address address (or a value
;   of EAX register for void functions).
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
; @param args       Arguments.
;
; @return
;   A value returned by the called function (or a value of EAX register for
;   functions of 'void' type.
;-----------------------------------------------------------------------------*/
uint32_t ExternalProcess::call_cdecl_function(uint32_t address, uint32_t argc,
                                              uint32_t args, ...)
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
        *reinterpret_cast<uint32_t *>(local_caller_buffer + i * 5 + 1) =
            *(&args + i);
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

    /* Create a thread in the remote process */
    HANDLE thread_handle = CreateRemoteThread(
        static_cast<HANDLE>(_handle), NULL, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(caller_address), NULL, 0,
        NULL);

    /* Wait for a return from the function */
    while (WaitForSingleObject(thread_handle, 0) != 0)
    {
    }

    DWORD result = 0;
    /* Get the returned value */
    GetExitCodeThread(thread_handle, &result);

    CloseHandle(thread_handle);

    free(caller_address);

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
    HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
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
    CloseHandle(snapshot_handle);
    return result;
}

/**
 * @file external_process.hpp
 *
 * @brief Declaration of ExternalProcess class. This class provides
 *        functionality for interacting with external Win32 processess.
 *
 */
#ifndef EXTERNAL_PROCESS_HPP
#define EXTERNAL_PROCESS_HPP

#include <cstdint>
#include <unordered_map>

namespace P1ExternalProcess
{

enum class enInjectionType
{
    EIT_JMP = 1,
    EIT_PUSHRET,
};

enum enVirtualProtect
{
    NOACCESS = 0b0001,
    READ = 0b0010,
    WRITE = 0b0100,
    EXECUTE = 0b1000,
    READ_EXECUTE = READ | EXECUTE,
    READ_WRITE = READ | WRITE,
    READ_WRITE_EXECUTE = READ | WRITE | EXECUTE
};

class ExternalProcess
{
public:
    /**
     * @brief This constructor takes a process id @process_id, gets the process
     *        handle using OpenProcess method and assigns it to @_handle field.
     *
     * @param process_id    Process id.
     */
    ExternalProcess(uint32_t process_id);

    /**
     * @brief This constructor takes a process name @process_name, gets the
     *        process id using @get_process_id_by_process_name method and
     *        delegates object creation to the constructor that takes a process
     *        id.
     *
     * @param process_name    Process name.
     */
    ExternalProcess(const char *process_name);

    /**
     * @brief Removes all created external callers and injected code from remote
     *        process memory. Frees all memory allocated in remote process.
     *        Closes process handle.
     */
    ~ExternalProcess();

    /**
     * @brief Reads @size bytes at @address address of the external process.
     *        Writes the result to @out_result.
     *
     * @param address    Address to read from.
     * @param size       Number of bytes to read.
     * @param out_result Buffer to write the result to.
     */
    void read_buf(uint32_t address, uint32_t size, void *out_result) const;

    /**
     * @brief Writes @size bytes from @data buffer to the external process at
     *        @address address.
     *
     * @param address    Address to write to.
     * @param size       Number of bytes to write.
     * @param data       Buffer to read data from.
     */
    void write_buf(uint32_t address, uint32_t size, const void *data) const;

    /**
     * @brief Allocates @size bytes in the external process.
     *
     * If allocation is successful, adds an entry to the @_allocated_memory map,
     * where the key is the address where the memory was allocated, the value is
     * the number of allocated bytes.
     *
     * @param size  Number of bytes to allocate.
     *
     * @return
     *   Address of the allocated memory if success.
     *   NULL if fail.
     */
    uint32_t alloc(const uint32_t size);

    /**
     * @brief Frees allocated region located at @address address in the external
     *        process address space.
     *
     * Also, removes an entry from the @_allocated_memory map, where the key is
     * @address.
     *
     * @param address    Address of the region to free.
     */
    void free(uint32_t address);

    /**
     * @brief Changes the protection on a region of committed pages in the
     *        virtual address space of the calling process.
     *
     * @param address    Region address.
     * @param size       Region size.
     * @param type       Protection type.
     */
    void set_virtual_protect(uint32_t address, uint32_t size,
                             enVirtualProtect type);

    /**
     * @brief Restores the original protection on a region of committed pages in
     *        the virtual address space of the calling process.
     *
     * @param address    Region address.
     */
    void restore_virtual_protect(uint32_t address);

    /**
     * @brief Return an address of the module named @module_name (if exists).
     *
     * @param module_name    Module name.
     *
     * @return
     *   Non-zero module address if success.
     *   0 if failire.
     */
    uint32_t get_module_address(const char *module_name);

    /**
     * @brief Calls a function at @address address with @args arguments using
     *       'cdecl' call convention.
     *
     * Allocates a buffer with execution rights in the remote process.
     * Writes to this buffer a set of x86 instructions that call a function at
     * @address with @args arguments using 'cdecl' call convention.
     * Creates a thread that executes the code in this buffer.
     * Waits for this thread to finish executing.
     * Returns a value returned by the function at the @address address (or
     * value of EAX register for void functions).
     *
     * @param address   An address of the function to be called.
     * @param argc      A number of arguments that the function takes.
     * @param args      Function arguments (if any).
     *
     * @return
     *   A value returned by the called function (or a value of EAX register for
     *   functions of 'void' type).
     */
    uint32_t call_cdecl_function(uint32_t address, uint32_t argc, ...);

    /**
     * @brief Calls a function at @address address with @args arguments using
     *       'stdcall' call convention.
     *
     * Allocates a buffer with execution rights in the remote process.
     * Writes to this buffer a set of x86 instructions that call a function at
     * @address with @args arguments using 'stdcall' call convention.
     * Creates a thread that executes the code in this buffer.
     * Waits for this thread to finish executing.
     * Returns a value returned by the function at the @address address (or
     * value of EAX register for void functions).
     *
     * @param address   An address of the function to be called.
     * @param argc      A number of arguments that the function takes.
     * @param args      Function arguments (if any).
     *
     * @return
     *   A value returned by the called function (or a value of EAX register
     * for functions of 'void' type).
     */
    uint32_t call_stdcall_function(uint32_t address, uint32_t argc, ...);

    /**
     * @brief Calls a function at @address address with @args arguments using
     *        'thiscall' call convention.
     *
     * Allocates a buffer with execution rights in the remote process.
     * Writes to this buffer a set of x86 instructions that call a function at
     * @address with @args arguments using 'thiscall' call convention.
     * Creates a thread that executes the code in this buffer.
     * Waits for this thread to finish executing.
     * Returns a value returned by the function at the @address address (or
     * value of EAX register for void functions).
     *
     * @param address   An address of the function to be called.
     * @param this_ptr  A pointer to an object for which the @ec caller will
     *                  call method at @ec.function_address address.
     * @param argc      A number of arguments that the function takes.
     * @param args      Function arguments (if any).
     *
     * @return A value returned by the called function (or a value of EAX
     *         register for functions of 'void' type).
     */
    uint32_t call_thiscall_function(uint32_t address, uint32_t this_ptr,
                                    uint32_t argc, ...);

    /**
     * @brief Injects @bytes_size bytes of code into the external process at
     *        @address address.
     *
     * @param address               An address where code should be injected.
     * @param bytes                 Code bytes to be injected at @address
     *                              address.
     * @param bytes_size            Number of the bytes to be injected.
     * @param overwrite_bytes_size  Number of original bytes to be overwritten
     *                              and executed after injected code.
     * @param it                    Type of transition to the injected code.
     */
    void inject_code(uint32_t address, const uint8_t *bytes,
                     uint32_t bytes_size, uint32_t overwrite_bytes_size,
                     enInjectionType it);

    /**
     * @brief Removes code injected at @address address (if any), restores the
     *        original bytes overwritten by the injector.
     *
     * @param address   An address where the code was injected.
     */
    void uninject_code(uint32_t address);

    /**
     * @brief Patches the external process memory with @size bytes of @bytes
     *        code at @address address.
     *
     * @param address   An address where the code should be patched.
     * @param bytes     Code bytes to be patched at @address address.
     * @param size      Number of the bytes to be patched.
     */
    void patch(uint32_t address, const uint8_t *bytes, uint32_t size);

    /**
     * @brief Searches for a sequence of bytes in the external process.memory.
     *
     * @param address   The address from which to start the search.
     * @param size      The size of the memory area in which to search for the
     *                  signature.
     * @param signature The byte sequence to be found.
     * @param mask      The mask to search by. The character 'x' is a complete
     *                  match. Characters other than 'x' ignore the
     *                  corresponding byte.
     *
     * @return The address of the first occurrence of @signature by mask @mask
     * in memory area [@address ... @address + @size), or zero on unsuccessful
     * search.
     */
    uint32_t find_signature(uint32_t address, uint32_t size,
                            const uint8_t *signature, const char *mask) const;

    /** @brief Reads a value of type T from the specified address in the
     *        external process' memory.
     *
     * @tparam T        A type of the value to be read.
     * @param address   An address to read the value from.
     *
     * @return A value read from the specified address.
     */
    template <typename T> T read(uint32_t address) const;

    /** @brief Writes a value of type T to the specified address in the
     *        external process' memory.
     *
     * @tparam T        A type of the value to be written.
     * @param address   An address to write the value to.
     * @param data      A value to be written.
     */
    template <typename T> void write(uint32_t address, const T &data) const;

private:
    enum class enCallConvention
    {
        ECC_CDECL = 1,
        ECC_STDCALL,
        ECC_THISCALL
    };
    struct VirtualProtect
    {
        uint32_t size;
        uint32_t original_protect;
    };
    struct ExternalCaller
    {
        uint32_t function_address;
        uint32_t caller_address;
        uint32_t argc;
        enCallConvention cc;
    };
    struct InjectedCodeInfo
    {
        uint32_t injected_bytes_number;
        uint32_t overwritten_bytes_number;
        uint32_t allocated_buffer;
    };

    /**
     * @brief Return process id of process named @process_name (if one is
     *        running).
     *
     * @param process_name  Process name.
     *
     * @return
     *   Process id if success.
     *   0 if failire.
     */
    uint32_t get_process_id_by_process_name(const char *process_name) const;

    /**
     * @brief Adds debug access rights to the external process.
     *
     * // TODO: Test.
     */
    void provide_debug_access(void);

    /**
     * @brief Allocates and initializes a buffer in a remote process for calling
     * functions using'cdecl' call convention.
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
     * @param address   An address of a function to be called.
     * @param argc      A number of arguments that the function takes.
     *
     * @return Address where the caller is placed in the external process.
     */
    uint32_t build_cdecl_caller(uint32_t address, uint32_t argc);

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
     * @param address   An address of the function to be called.
     * @param argc      A number of arguments that the function takes.
     *
     * @return Address where the caller is placed in the external process.
     */
    uint32_t build_stdcall_caller(uint32_t address, uint32_t argc);

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
     * @param address   An address of the function to be called.
     * @param argc      A number of arguments that the function takes.
     *
     * @return Address where the caller is placed in the external process.
     */
    uint32_t build_thiscall_caller(uint32_t address, uint32_t argc);

    /**
     * @brief
     * Writes arguments @args with which the function should be called. Since
     * instructions for pushing arguments on stack for all implemented callers
     * (cdecl, thiscall, stdcall) are at the very beginning, this function is
     * universal and applicable to callers of all call supported call
     * conventions.
     *
     * @param ec    A caller to specify arguments for.
     * @param args  Arguments.
     */
    void send_external_caller_arguments(const ExternalCaller &ec, uint32_t args,
                                        ...);

    /**
     * @brief
     * Puts @this_ptr in caller @ec. The ffset is calculated as follows:
     *  @ec.argc * 5 is a push args instructions size. +1 is a MOV ECX
     * instruction size. It must be followed by @this_ptr.
     *
     * @param ec        A caller to specify class object ptr for.
     * @param this_ptr  A pointer to an object for which the @ec caller will
     *                  call method at @ec.function_address address.
     */
    void send_thiscall_this_ptr(const ExternalCaller &ec, uint32_t this_ptr);

    /**
     * @brief Calls a function using information from a caller.
     *
     * Creates a thread in remote process. This thread executes the code located
     * in @ec.caller_address buffer. Waits for this thread to finish executing.
     * Returns a value returned by the function at the @ec.function+address
     * address (or a value of EAX register for void functions).
     *
     * @param   eс Caller whose function is to be called.
     *
     * @return  A value returned by the called function (or a value of EAX
     *          register for void functions.
     */
    uint32_t call_external_function(const ExternalCaller &ec) const;

    /**
     * @brief Injects @bytes_size bytes of code from the @bytes argument into
     *        remote process at @address address.
     *
     * Logic:
     * Allocates a @allocated_buf buffer in a remote process with execute
     * permissions.
     * Writes @bytes injected code to it. Writes an unconditional jump
     * instruction 'JMP'to @allocated_buf at @address. This JMP instruction
     * overwrites 5 bytes at @address. These 5 bytes are recovered in the
     * @allocated_buf buffer after the injected code.
     * There are cases where the fifth byte (@address+4) is not the last byte of
     * the instruction. In this case, the number of overwritten bytes must be
     * specified explicitly (argument @overwrite_bytes_size) and all of them
     * will be executed in the @allocated_buf buffer after the injected code.
     * If @overwrite_bytes_size is greater than 5, then to save addressing, the
     * first (@overwrite_bytes_size - 5) bytes at @address will be replaced with
     * nops (0x90).
     * In the example below, there is a situation when it is impossible to write
     * a JMP instruction at @address without explicitly specifying
     * @overwrite_bytes_size.
     *
     * Original bytes:
     *  ADDRESS                           INSTRUCTION(S)      INSTRUCTION(S) LEN
     *  @address + 00                     instruction #1      4
     *  @address + 04                     instruction #2      2
     *  @address + 06                     instruction #3      4
     *
     * After injection with @overwrite_bytes_size = 6:
     *  ADDRESS                           INSTRUCTION(S)      INSTRUCTION(S) LEN
     *  @address + 00                     nop                 1
     *  @address + 01                     jmp @allocated_buf  5
     *  @address + 06                     instruction #3      4
     *
     * Allocated buffer:
     *  ADDRESS                           INSTRUCTION(S)      INSTRUCTION(S) LEN
     *  @allocated_buf + 00               injected code       @size
     *  @allocated_buf + @size            instruction #1      4
     *  @allocated_buf + @size + 4        instruction #2      2
     *  @allocated_buf + @size + 4 + 2    jmp @address + 06   5
     *
     * @param address               An address where code should be injected.
     * @param bytes                 Code bytes to be injected at @address
     *                              address.
     * @param bytes_size            Number of the bytes to be injected.
     * @param overwrite_bytes_size  The number of original bytes to be
     *                              overwritten and executed after injected code
     *
     * @return  allocated buffer address @allocated_buf
     */
    uint32_t inject_code_using_jmp(uint32_t address, const uint8_t *bytes,
                                   uint32_t bytes_size,
                                   uint32_t overwrite_bytes_size);

    /**
     * @brief Injects @bytes_size bytes of code from the @bytes argument into
     * remote process at @address address.
     *
     * @param address               An address where code should be injected.
     * @param bytes                 Code bytes to be injected at @address
     *                              address.
     * @param bytes_size            Number of the bytes to be injected.
     * @param overwrite_bytes_size  The number of original bytes to be
     *                              overwritten and executed after injected code
     *
     * @return  allocated buffer address @allocated_buf
     */
    uint32_t inject_code_using_push_ret(uint32_t address, const uint8_t *bytes,
                                        uint32_t bytes_size,
                                        uint32_t overwrite_bytes_size);

    void *_handle;
    void *_dbg_handle;
    uint32_t _process_id;
    std::unordered_map<uint32_t, uint32_t> _allocated_memory;
    std::unordered_map<uint32_t, VirtualProtect> _virtual_protect;
    std::unordered_map<uint32_t, ExternalCaller> _callers;
    std::unordered_map<uint32_t, InjectedCodeInfo> _injected_code;
};

template <typename T> inline T ExternalProcess::read(uint32_t address) const
{
    T result;
    read_buf(address, sizeof(T), &result);
    return result;
}

template <typename T>
inline void ExternalProcess::write(uint32_t address, const T &data) const
{
    write_buf(address, sizeof(data), &data);
}

} /* namespace P1ExternalProcess */

#endif /* EXTERNAL_PROCESS_HPP */

/**
 * @file ep_core.h
 * @brief Core external process interaction functionality.
 */
#ifndef EP_CORE_H_
#define EP_CORE_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#include <stdbool.h>
#include <stdint.h>

typedef struct EPCProcess EPCProcess;

typedef enum EPCVirtualProtect
{
    EPCVP_EMPTY = 0,
    EPCVP_READ = 0b1,
    EPCVP_WRITE = 0b10,
    EPCVP_EXECUTE = 0b100,
} EPCVirtualProtect;

typedef enum EPCCallConvention
{
    EPCCC_INVALID = 0,
    EPCCC_CDECL = 1,
    EPCCC_STDCALL = 2,
    EPCCC_THISCALL_MSVC = 3,
    EPCCC_FASTCALL = 4,
    EPCCC_COUNT_,
} EPCCallConvention;

typedef struct EpCoreFunctionCaller EpCoreFunctionCaller;

/**
 * @brief Creates a new Windows process interaction object.
 * @param[out] out_process The process object to write to.
 * @return true if the process was created successfully.
 * @return false if the process could not be created.
 */
bool epc_create(EPCProcess **out_process);

/**
 * @brief Destroys a Windows process interaction object.
 * @param[in] process The process object to destroy.
 * @return true if the process was destroyed successfully.
 * @return false if the process could not be destroyed.
 */
bool epc_destroy(EPCProcess *process);

/**
 * @brief Opens a Windows process by its ID.
 *
 * @param[in] process_id The ID of the process to open.
 * @param[out] out_process The process object to write to.
 * @return true if the process was opened successfully.
 * @return false if the process could not be opened.
 */
bool epc_open_process(EPCProcess *process, unsigned int process_id);

/**
 * @brief Closes a Windows process by its name.
 * @param[in] process The process object to close.
 */
bool epc_close_process(EPCProcess *process);

/**
 * @brief Returns process ID by its name.
 * @param[in] process_name The name of the process to find.
 * @param[out] out_process_id The ID of the process to write to.
 * @return true if the process was found.
 * @return false if the process was not found.
 */
bool epc_get_process_id_by_name(const char *process_name,
                                unsigned int *out_process_id);

/**
 * @brief Reads \p size bytes from \p address in the \p process process.
 * @param[in] process The process to read from.
 * @param[in] address The address to read from.
 * @param[in] size The number of bytes to read.
 * @param[out] out_result The buffer to write the result to.
 * @return true if the read was successful.
 */
bool epc_read_buf(const EPCProcess *process, uintptr_t address, size_t size,
                  void *out_result);

bool epc_read_buf_nt(const EPCProcess *process, uintptr_t address, size_t size,
                     void *out_result);

/**
 * @brief Writes \p size bytes to \p address in the \p process process.
 * @param[in] process The process to write to.
 * @param[in] address The address to write to.
 * @param[in] size The number of bytes to write.
 * @param[in] data The data to write.
 * @return true if the write was successful.
 * @return false if the write was not successful.
 */
bool epc_write_buf(const EPCProcess *process, uintptr_t address, size_t size,
                   const void *data);

bool epc_write_buf_nt(const EPCProcess *process, uintptr_t address, size_t size,
                      const void *data);

/**
 * @brief Allocates \p size bytes in the \p process process.
 * @param[in] process The process to allocate in.
 * @param[in] size The number of bytes to allocate.
 * @param[out] out_address The address of the allocated memory.
 * @return true if the allocation was successful.
 * @return false if the allocation was not successful.
 */
bool epc_alloc(const EPCProcess *process, size_t size, uintptr_t *out_address);

bool epc_alloc_nt(const EPCProcess *process, size_t size,
                  uintptr_t *out_address);

/**
 * @brief Frees \p size bytes at \p address in the \p process process
 * @param[in] process The process to free in.
 * @param[in] address The address to free.
 * @return true if the free was successful.
 * @return false if the free was not successful.
 */
bool epc_free(const EPCProcess *process, uintptr_t address);

bool epc_free_nt(const EPCProcess *process, uintptr_t address);

/**
 * @brief Changes the memory protection of \p size bytes at \p address in the
 * \p process .
 * @param[in] process The process to change the protection in.
 * @param[in] address The address to change the protection of.
 * @param[in] size The number of bytes to change the protection of.
 * @param[in] protect The new protection.
 * @param[out] out_old_protect The old protection.
 * @return true if the protection was changed successfully.
 * @return false if the protection was not changed successfully.
 */
bool epc_virtual_protect(const EPCProcess *process, uintptr_t address,
                         size_t size, EPCVirtualProtect protect,
                         EPCVirtualProtect *out_old_protect);

bool epc_virtual_protect_nt(const EPCProcess *process, uintptr_t address,
                            size_t size, EPCVirtualProtect protect,
                            EPCVirtualProtect *out_old_protect);

/**
 * @brief Returns the address of a module named \p module_name in the \p process
 * @param[in] process The process to search in.
 * @param[in] module_name The name of the module to search for.
 * @param[out] out_address The address of the module.
 * @return true if the module was found.
 * @return false if the module was not found.
 */
bool epc_get_module_address(const EPCProcess *process, const char *module_name,
                            uintptr_t *out_address);

/**
 * @brief Builds instructions to call a remote process' \p process function
 * located at \p function_address address and acceping \p argc arguments. Writes
 * the built instructions to remote process' memory and returns a caller object
 * which can be used to pass arguments and call the function.
 * @param[in] process The process to build the caller in.
 * @param[in] function_address The address of the function to call.
 * @param[in] call_convention The calling convention of the function.
 * @param[in] argc The number of arguments the function accepts.
 * @param[out] out_caller The caller object to write to.
 * @return true if the caller was built successfully.
 * @return false if the caller was not built successfully
 */
bool epc_function_caller_build(const EPCProcess *process,
                               uintptr_t function_address,
                               EPCCallConvention call_convention, size_t argc,
                               EpCoreFunctionCaller **out_caller);

/**
 * @brief Destroys a function caller object.
 * @param[in] caller The caller to destroy.
 * @return true if the caller was destroyed successfully.
 * @return false if the caller was not destroyed successfully.
 */
bool epc_function_caller_destroy(EpCoreFunctionCaller *caller);

/**
 * @brief Sends \p argc arguments for the \p caller function caller.
 * For class methods, the first argument should be the class instance.
 * @param[in] caller The caller to send the arguments for.
 * @param[in] argc The number of arguments to send.
 * @param[in] ... The arguments to send.
 * @return true if the arguments were sent successfully.
 * @return false if the arguments were not sent successfully.
 */
bool epc_function_caller_send_args(const EpCoreFunctionCaller *caller,
                                   size_t argc, ...);

/**
 * @brief Calls the function of the \p caller function caller. Waits (if
 * required) for the function to return and returns the result using
 * \p out_result.
 * @param[in] caller The caller to call.
 * @param[out] out_result The result of the function call.
 * @param[in] wait_for_return Whether to wait for the function to return.
 * @return true if the function was called successfully.
 * @return false if the function was not called successfully.
 */
bool epc_function_caller_call(const EpCoreFunctionCaller *caller,
                              uintptr_t *out_result, bool wait_for_return);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EP_CORE_H_ */

/**-----------------------------------------------------------------------------
; @file external_process.hpp
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#ifndef EXTERNAL_PROCESS_HPP
#define EXTERNAL_PROCESS_HPP

#include <cstdint>
#include <unordered_map>

namespace P1ExternalProcess
{

class ExternalProcess
{
public:
    ExternalProcess(void *process_handle);
    ExternalProcess(uint32_t process_id);
    ExternalProcess(const char *process_name);
    ~ExternalProcess(void);
    void read_buf(uint32_t address, uint32_t size, void *out_result) const;
    void write_buf(uint32_t address, uint32_t size, const void *data) const;
    uint32_t alloc(const uint32_t size);
    void free(uint32_t address);
    uint32_t call_cdecl_function(uint32_t address, uint32_t argc, uint32_t args,
                                 ...);
    uint32_t call_stdcall_function(uint32_t address, uint32_t argc,
                                   uint32_t args, ...);
    uint32_t call_thiscall_function(uint32_t address, uint32_t this_ptr,
                                    uint32_t argc, uint32_t args, ...);
    template <typename T> T read(uint32_t address) const;
    template <typename T> void write(uint32_t address, const T &data) const;

private:
    enum class enCallConvention
    {
        ECC_CDECL = 1,
        ECC_STDCALL,
        ECC_THISCALL
    };
    struct ExternalCaller
    {
        uint32_t function_address;
        uint32_t caller_address;
        uint32_t argc;
        enCallConvention cc;
    };

    uint32_t get_process_id_by_process_name(const char *process_name) const;
    uint32_t build_cdecl_caller(uint32_t address, uint32_t argc);
    uint32_t build_stdcall_caller(uint32_t address, uint32_t argc);
    uint32_t build_thiscall_caller(uint32_t address, uint32_t argc);
    void send_external_caller_arguments(const ExternalCaller &ec, uint32_t args,
                                        ...);
    void send_thiscall_this_ptr(const ExternalCaller &ec, uint32_t this_ptr);
    uint32_t call_external_function(const ExternalCaller &ec) const;

    void *_handle;
    std::unordered_map<uint32_t, uint32_t> _allocated_memory;
    std::unordered_map<uint32_t, ExternalCaller> _callers;
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

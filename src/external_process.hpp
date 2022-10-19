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

private:
    uint32_t get_process_id_by_process_name(const char *process_name) const;

    void *_handle;
    std::unordered_map<uint32_t, uint32_t> _allocated_memory;
};

template <typename T> inline T ExternalProcess::read(uint32_t address) const
{
    T result;
    read_buf(address, sizeof(T), &result);
    return result;
}

} /* namespace P1ExternalProcess */

#endif /* EXTERNAL_PROCESS_HPP */

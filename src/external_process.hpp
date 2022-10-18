/**-----------------------------------------------------------------------------
; @file external_process.hpp
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#ifndef EXTERNAL_PROCESS_HPP
#define EXTERNAL_PROCESS_HPP

#include <cstdint>

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

private:
    uint32_t get_process_id_by_process_name(const char *process_name) const;

    void *_handle;
};

} /* namespace P1ExternalProcess */

#endif /* EXTERNAL_PROCESS_HPP */

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
    ExternalProcess(uint32_t process_id);
    ExternalProcess(const char *process_name);
    ~ExternalProcess(void);

private:
    uint32_t get_process_id_by_process_name(const char *process_name) const;

    uint32_t _process_id;
};

} /* namespace P1ExternalProcess */

#endif /* EXTERNAL_PROCESS_HPP */

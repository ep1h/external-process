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
    ~ExternalProcess(void);

private:
    uint32_t _process_id;
};

} /* namespace P1ExternalProcess */

#endif /* EXTERNAL_PROCESS_HPP */

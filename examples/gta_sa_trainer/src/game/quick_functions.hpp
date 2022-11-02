/**-----------------------------------------------------------------------------
; @file test_utils.cpp
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <cstdint>
#include "utils.hpp"

namespace QuickFunctions
{

vec3f getBonePosition_sub_5E4280(uint32_t ped_addr, uint32_t bone_id,
                                 uint8_t is_dynamic);

}

#endif /* FUNCTIONS_HPP */

/**
 * @file quick_functions.hpp
 *
 */
#ifndef QUICK_FUNCTIONS_HPP
#define QUICK_FUNCTIONS_HPP

#include <cstdint>
#include "utils.hpp"

namespace QuickFunctions
{

vec3f getBonePosition_sub_5E4280(uint32_t ped_addr, uint32_t bone_id,
                                 uint8_t is_dynamic);

}

#endif /* QUICK_FUNCTIONS_HPP */

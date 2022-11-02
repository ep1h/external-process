/**
 * @file test_utils.cpp
 *
 */
#include "utils.hpp"
#include <math.h>

float calculate_distance_2d(const vec2f &a, const vec2f &b)
{
    return sqrtf(powf(a.x - b.x, 2.0) + powf(a.y - b.y, 2.0f));
}

float calculate_distance_3d(const vec3f &a, const vec3f &b)
{
    return sqrtf(powf(a.x - b.x, 2.0) + powf(a.y - b.y, 2.0f) +
                 powf(a.z - b.z, 2.0f));
}

/**
 * @file test_utils.cpp
 *
 */
#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>

struct vec2ui
{
    uint32_t x;
    uint32_t y;
};

struct vec2f
{
    float x;
    float y;
};

struct vec3f
{
    float x;
    float y;
    float z;
};

struct mat44f
{
    float m[4][4];
};

float calculate_distance_2d(const vec2f& a, const vec2f& b);
float calculate_distance_3d(const vec3f& a, const vec3f& b);

#endif /* UTILS_HPP */

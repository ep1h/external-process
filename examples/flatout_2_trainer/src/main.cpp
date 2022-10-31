/**
 * @file main.cpp
 *
 * @brief This file contains the source code for a basic FlatOut 2 game trainer,
 *        demonstrating the capabilities of the ExternalProcess class using a
 *        practical example with room for improvement.
 *
 * Usage:
 *   F1 - Repair the player's vehicle.
 *   F2 - Enhance acceleration performance.
 *   F3 - Boost nitro efficiency.
 *   F4 - Eliminate opponents.
 *   F5 - Switch positions with the closest opponent.
 *   LEFT/RIGHT/UP/DOWN arrows - Reposition opponents.
 *
 */
#include "../../../src/external_process.hpp"
#include <windows.h>
#include <math.h>

using namespace P1ExternalProcess;

enum enActors
{
    PLAYER,
    ENEMY_1,
    ENEMY_2,
    ENEMY_3,
    ENEMY_4,
    ENEMY_5,
    ENEMY_6,
    ENEMY_7
};

enum enVehInfoOffsets
{
    VEH_NAME = 0x40,
    POS_Y = 0x1E0,
    POS_Z = 0x1E4,
    POS_X = 0x1E8,
    VEL_Y = 0x280,
    VEL_Z = 0x284,
    VEL_X = 0x288,
    VEL_ROLL = 0x290,
    VEL_YAW = 0x294,
    VEL_PITCH = 0x298,
    ACCELERATION_FORCE = 0x5C4,
    NITRO_ACCELERATION_FORCE = 0x5C8,
    NITRO = 0x5CC,
    MAX_NITRO = 0x5D0, /* default 5.0f */
    HEALTH = 0x6AA0    /* 0.0f - max, 1.0f - min*/
};

struct vec3f
{
    float x;
    float y;
    float z;
};

ExternalProcess fo2("FlatOut2.exe");

uint32_t get_veh_info(enActors actor)
{
    uint32_t base_address = fo2.get_module_address("FlatOut2.exe");
    uint32_t global_struct = fo2.read<uint32_t>(base_address + 0x296DC8);
    uint32_t actors_ptr_array = fo2.read<uint32_t>(global_struct + 0x14);
    uint32_t player_actor_info =
        fo2.read<uint32_t>(actors_ptr_array + actor * 4);
    uint32_t vehicle_info = fo2.read<uint32_t>(player_actor_info + 0x33C);
    return vehicle_info;
}

void repair(void)
{
    uint32_t player_veh = get_veh_info(PLAYER);
    fo2.write<float>(player_veh + HEALTH, 0.0f);
}

void super_acceleration(void)
{
    uint32_t player_veh = get_veh_info(PLAYER);
    float cur = fo2.read<float>(player_veh + ACCELERATION_FORCE);
    if (cur < 5.0f)
    {
        fo2.write<float>(player_veh + ACCELERATION_FORCE, 5.0f);
    }
}

void super_nitro(void)
{
    uint32_t player_veh = get_veh_info(PLAYER);
    float cur = fo2.read<float>(player_veh + NITRO_ACCELERATION_FORCE);
    if (cur < 5.0f)
    {
        fo2.write<float>(player_veh + NITRO_ACCELERATION_FORCE, 5.0f);
    }
}

void destroy_opponents(void)
{
    for (int i = 1; i < 8; i++)
    {
        uint32_t veh = get_veh_info((enActors)i);
        fo2.write<float>(veh + VEL_Z, -30.0f);
    }
}

float calculate_distance_3d(const vec3f &a, const vec3f &b)
{
    return sqrtf(powf(a.x - b.x, 2.0) + powf(a.y - b.y, 2.0f) +
                 powf(a.z - b.z, 2.0f));
}

void swap_with_nearest_vehicle(void)
{
    uint32_t player_veh = get_veh_info(PLAYER);
    vec3f player_pos = fo2.read<vec3f>(player_veh + POS_Y);
    uint32_t nearest_veh = 0;
    float nearest_dist = 999999.0f;
    for (int i = 1; i < 8; i++)
    {
        uint32_t veh = get_veh_info((enActors)i);
        vec3f pos = fo2.read<vec3f>(veh + POS_Y);
        float dist = calculate_distance_3d(player_pos, pos);
        if (dist < nearest_dist)
        {
            nearest_dist = dist;
            nearest_veh = veh;
        }
    }
    vec3f pos = fo2.read<vec3f>(nearest_veh + POS_Y);
    vec3f vel = fo2.read<vec3f>(nearest_veh + VEL_Y);
    vec3f vel_rot = fo2.read<vec3f>(nearest_veh + VEL_ROLL);

    vec3f player_vel = fo2.read<vec3f>(player_veh + VEL_Y);
    vec3f player_vel_rot = fo2.read<vec3f>(player_veh + VEL_ROLL);

    fo2.write<vec3f>(nearest_veh + POS_Y, player_pos);
    fo2.write<vec3f>(nearest_veh + VEL_Y, player_vel);
    fo2.write<vec3f>(nearest_veh + VEL_ROLL, player_vel_rot);

    fo2.write<vec3f>(player_veh + POS_Y, pos);
    fo2.write<vec3f>(player_veh + VEL_Y, vel);
    fo2.write<vec3f>(player_veh + VEL_ROLL, vel_rot);
}

void cc(void)
{
    if (GetAsyncKeyState(VK_UP))
    {
        for (int i = 1; i < 8; i++)
        {
            uint32_t veh = get_veh_info((enActors)i);
            float v = fo2.read<float>(veh + VEL_Z);
            fo2.write<float>(veh + VEL_Z, v + 10.0f);
            fo2.write<float>(veh + VEL_X, 0.0f);
            fo2.write<float>(veh + VEL_Y, 0.0f);
        }
    }
    if (GetAsyncKeyState(VK_DOWN))
    {
        for (int i = 1; i < 8; i++)
        {
            uint32_t veh = get_veh_info((enActors)i);
            float v = fo2.read<float>(veh + VEL_Z);
            fo2.write<float>(veh + VEL_Z, v - 10.0f);
            // fo2.write<float>(veh + VEL_X, 0.0f);
            // fo2.write<float>(veh + VEL_Y, 0.0f);
        }
    }
    if (GetAsyncKeyState(VK_LEFT))
    {
        for (int i = 1; i < 8; i++)
        {
            uint32_t veh = get_veh_info((enActors)i);
            float v = fo2.read<float>(veh + VEL_Z);
            fo2.write<float>(veh + VEL_X, v - 2.0f);
            fo2.write<float>(veh + VEL_ROLL, 30.0f);
        }
    }
    if (GetAsyncKeyState(VK_RIGHT))
    {
        for (int i = 1; i < 8; i++)
        {
            uint32_t veh = get_veh_info((enActors)i);
            float v = fo2.read<float>(veh + VEL_Z);
            fo2.write<float>(veh + VEL_X, v + 2.0f);
            fo2.write<float>(veh + VEL_ROLL, -30.0f);
        }
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    while (true)
    {
        if (GetAsyncKeyState(VK_F1))
        {
            repair();
        }
        if (GetAsyncKeyState(VK_F2))
        {
            super_acceleration();
        }
        if (GetAsyncKeyState(VK_F3))
        {
            super_nitro();
        }
        if (GetAsyncKeyState(VK_F4))
        {
            destroy_opponents();
        }
        if (GetAsyncKeyState(VK_F5))
        {
            swap_with_nearest_vehicle();
        }
        cc();
        Sleep(100);
    }
    return 0;
}

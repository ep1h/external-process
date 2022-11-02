/**
 * @file main.cpp
 *
 * @brief This file contains the source code for a basic GTA SA game trainer,
 *        demonstrating the capabilities of the ExternalProcess class using a
 *        practical example with room for improvement.
 *
 */
#include <iostream>
#include <chrono>
#include <windows.h>

#include "game/gta_sa.hpp"
#include "../utils.hpp"

GtaSA gta;

void get_bone_position_benchmark(void)
{
    uint32_t *streamed_peds = nullptr;
    uint32_t peds_number = gta.get_streamed_peds(&streamed_peds);
    if (peds_number == 0)
    {
        return;
    }

    auto start = std::chrono::steady_clock::now();
    for (volatile uint32_t i = 0; i < 1000; i++)
    {
        for (uint32_t j = 0; j < peds_number; j++)
        {
            volatile vec3f bone_pos =
                gta.get_bone_position(streamed_peds[j], 6, 0);
            (void)bone_pos;
        }
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << 1000 * peds_number
              << " calls of get_bone_position (1000 time for " << peds_number
              << " peds): "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << " microseconds." << std::endl;

    start = std::chrono::steady_clock::now();
    for (volatile uint32_t i = 0; i < 1000; i++)
    {
        for (uint32_t j = 0; j < peds_number; j++)
        {
            volatile vec3f bone_pos =
                gta.get_bone_position_quick(streamed_peds[j], 6, 0);
            (void)bone_pos;
        }
    }
    end = std::chrono::steady_clock::now();
    std::cout << 1000 * peds_number
              << " calls of get_bone_position_quick (1000 time for "
              << peds_number << " peds): "
              << std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                       start)
                     .count()
              << " microseconds." << std::endl;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    while (true)
    {
        Sleep(10);
        if (GetAsyncKeyState(0x31))
        {
            gta.auto_sprint_bug(true);
            gta.show_dialog_message("Auto sprint bug ~G~enabled", 1000);
            Sleep(500);
        }
        if (GetAsyncKeyState(0x32))
        {
            gta.auto_sprint_bug(false);
            gta.show_dialog_message("Auto sprint bug ~R~disabled", 1000);
            Sleep(500);
        }
        if (GetAsyncKeyState(0x33))
        {
            uint32_t player_ped = gta.get_player_ped();
            vec3f pos = gta.get_ped_pos(player_ped);
            vec3f bone_pos_1 = gta.get_bone_position(player_ped, 6, 0);
            vec3f bone_pos_2 = gta.get_bone_position_quick(player_ped, 6, 0);

            std::cout << "pos: " << pos.x << " " << pos.y << " " << pos.z
                      << std::endl;
            std::cout << "bone_pos_1: " << bone_pos_1.x << " " << bone_pos_1.y
                      << " " << bone_pos_1.z << std::endl;
            std::cout << "bone_pos_2: " << bone_pos_2.x << " " << bone_pos_2.y
                      << " " << bone_pos_2.z << std::endl;

            std::cout << std::endl;
            Sleep(500);
        }
        if (GetAsyncKeyState(0x34))
        {
            get_bone_position_benchmark();
            Sleep(500);
        }
        if (GetAsyncKeyState(0x35))
        {
            uint32_t player = gta.get_player_ped();
            gta.give_weapon(player, enWeaponId::WEAPON_MINIGUN, 1837, 0);
            Sleep(500);
        }
        if (GetAsyncKeyState(VK_F6))
        {
            uint8_t b = 0xC3;
            gta.patch(0x6FF410, &b, 1);
            float FOV = gta.read<float>(0x8D5038);
            FOV -= 0.5f;
            if ((int)FOV % 180 == 0)
            {
                FOV -= 0.5f;
            }
            gta.write<float>(0x8D5038, FOV);
            char msg[24];
            std::snprintf(msg, sizeof(msg), "FOV: ~R~%.1f", FOV);
            gta.show_dialog_message(msg, 1000);
        }
        if (GetAsyncKeyState(VK_F7))
        {
            uint8_t b = 0xC3;
            gta.patch(0x6FF410, &b, 1);
            float FOV = gta.read<float>(0x8D5038);
            FOV += 0.5f;
            if ((int)FOV % 180 == 0)
            {
                FOV += 0.5f;
            }
            gta.write<float>(0x8D5038, FOV);
            char msg[24];
            std::snprintf(msg, sizeof(msg), "FOV: ~G~%.1f", FOV);
            gta.show_dialog_message(msg, 1000);
        }
    }
    return 0;
}

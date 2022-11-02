/**
 * @file gta_sa.cpp
 *
 * @brief Implementation of the GtaSA class.
 *
 */
#include "gta_sa.hpp"
#include <cstring>

#include "quick_functions.hpp"

GtaSA::GtaSA(void) : ExternalProcess("gta_sa.exe")
{
    _ped_pool = read<uint32_t>(0xB74490);
    _ped_pool_objects = read<uint32_t>(_ped_pool);
    _ped_pool_bytemap = read<uint32_t>(_ped_pool + 4);
}

uint32_t GtaSA::get_player_ped(void)
{
    return read<uint32_t>(0xB6F5F0);
}

vec3f GtaSA::get_ped_pos(uint32_t ped)
{
    return read<vec3f>(read<uint32_t>(ped + 0x14) + 0x30);
}

float GtaSA::get_ped_health(uint32_t ped)
{
    return read<float>(ped + 0x540);
}

uint8_t GtaSA::get_ped_armed_weapon(uint32_t ped)
{
    return read<uint8_t>(ped + 0x740);
}

uint32_t GtaSA::get_ped_by_handle(uint32_t handle)
{
    // return call_cdecl_function(0x54FF90, 1, handle);
    uint8_t h_handle = read<uint8_t>((handle >> 8) + _ped_pool_bytemap);
    if (h_handle == (handle & 0x000000FF))
    {
        return _ped_pool_objects + 0x7C4 * (handle >> 8);
    }
    else
    {
        return 0;
    }
}

vec2ui GtaSA::get_screen_resolution(void)
{
    return read<vec2ui>(0xC17044);
}

vec2f GtaSA::get_crosshair_position(void)
{
    vec2f result = read<vec2f>(0xB6EC10);
    std::swap(result.x, result.y);
    return result;
}

float GtaSA::get_aspect_ratio(void)
{
    return read<float>(0xC3EFA4);
}

float GtaSA::get_cam_x_angle(void)
{
    return read<float>(0xB6F258);
}

float GtaSA::get_cam_y_angle(void)
{
    return read<float>(0xB6F248);
}

uint32_t GtaSA::get_streamed_peds(uint32_t **out_ped_ptr_array)
{
    static uint32_t peds[140]; // TODO: Read max peds number from 0xB74498
    *out_ped_ptr_array = peds;
    int i = 0;
    uint32_t v1 = _ped_pool_bytemap;
    for (uint32_t v2 = 0; v2 < 0x8b00; v2 += 0x100)
    {
        uint32_t handle = read<uint32_t>(v1);
        handle &= 0x000000FF;
        v1 += 1;
        if (handle < 0x80)
        {
            handle += v2;
            peds[i++] = get_ped_by_handle(handle);
        }
    }
    return i;
}

bool GtaSA::is_player_aiming(void)
{
    return get_onfoot_key_state(enOnfootKeys::AIM_WEAPON) & 0xFF;
}

mat44f GtaSA::get_view_matrix(void)
{
    return read<mat44f>(0xB6FA2C);
}

vec3f GtaSA::get_bone_position(uint32_t ped, uint32_t bone_id,
                               uint8_t is_dynamic)
{
    static uint32_t result_address = alloc(sizeof(vec3f));
    call_thiscall_function(0x5E4280, ped, 3, result_address, bone_id,
                           is_dynamic);
    return read<vec3f>(result_address);
}

vec3f GtaSA::get_bone_position_quick(uint32_t ped, uint32_t bone_id,
                                     uint8_t is_dynamic)
{
    return QuickFunctions::getBonePosition_sub_5E4280(ped, bone_id, is_dynamic);
}

vec3f GtaSA::get_screen_position(const vec3f &world_pos)
{
    /* 0x71DA00 */
    vec3f screen_pos;
    mat44f view = get_view_matrix();

    vec2ui screen_resolution = get_screen_resolution();

    screen_pos.x = (world_pos.z * view.m[2][0]) + (world_pos.y * view.m[1][0]) +
                   (world_pos.x * view.m[0][0]) + view.m[3][0];
    screen_pos.y = (world_pos.z * view.m[2][1]) + (world_pos.y * view.m[1][1]) +
                   (world_pos.x * view.m[0][1]) + view.m[3][1];
    screen_pos.z = (world_pos.z * view.m[2][2]) + (world_pos.y * view.m[1][2]) +
                   (world_pos.x * view.m[0][2]) + view.m[3][2];

    float recip = 1.0f / screen_pos.z;
    screen_pos.x *= (float)(recip * (int)screen_resolution.x);
    screen_pos.y *= (float)(recip * (int)screen_resolution.y);
    return screen_pos;
}

void GtaSA::give_weapon(uint32_t ped, enWeaponId weapon_id, uint32_t ammo,
                        uint32_t like_unused)
{
    uint32_t model_id = get_weapon_model_id(weapon_id);
    /* Load model */
    call_cdecl_function(0x4087E0, 2, model_id, 2);
    /* Give weapon */
    call_thiscall_function(0x5E6080, ped, 3, weapon_id, ammo, like_unused);
    /* Rrelease model */
    // call_cdecl_function(0x409C10, 1, model_id);
}

void GtaSA::show_dialog_message(const char *text, uint32_t time)
{
    /* Perhaps these manipulations are not necessary, since the page size is
     * enough for any text that can be displayed using the 0x69F1E0 function. */
    static uint32_t size = strlen(text);
    static uint32_t buf = alloc(size);
    if (size < strlen(text))
    {
        size = strlen(text);
        free(buf);
        buf = alloc(size);
    }
    write_buf(buf, size, text);
    call_cdecl_function(0x69F1E0, 4, (uint32_t)buf, time, 1, 1);
}

void GtaSA::unlock_fps(bool state)
{
    set_virtual_protect(0x53E94C, 1, enVirtualProtect::READ_WRITE_EXECUTE);
    write<uint8_t>(0x53E94C, state ? 0x00 : 0x0E);
    restore_virtual_protect(0x53E94C);
}

void GtaSA::auto_sprint_bug(bool state)
{
    set_virtual_protect(0x60A68E, 1, enVirtualProtect::READ_WRITE_EXECUTE);
    write<uint8_t>(0x60A68E, state ? 0x0E : 0x5E);
    restore_virtual_protect(0x60A68E);
}

uint16_t GtaSA::get_onfoot_key_state(enOnfootKeys id)
{
    return read<uint16_t>(0xB73458 + id * 2);
}

void GtaSA::set_onfoot_key_state(enOnfootKeys id, uint16_t state)
{
    write<uint16_t>(0xB73458 + id * 2, state);
}

void GtaSA::request_model(int32_t id, uint32_t flags)
{
    call_cdecl_function(0x4087E0, 2, id, flags);
}

void GtaSA::set_model_deletable(int32_t id)
{
    call_cdecl_function(0x409C10, 1, id);
}

uint32_t GtaSA::get_weapon_model_id(enWeaponId id)
{
    uint32_t result = 0;
    if (id >= 0 && id <= 9)
    {
        result += 330;
    }
    else if (id > 9 && id <= 15)
    {
        result += 311;
    }
    else if (id > 15 && id <= 29)
    {
        result += 326;
    }
    else if (id > 29 && id <= 31)
    {
        result += 325;
    }
    else if (id == 32)
    {
        result += 350;
    }
    else if (id > 32 && id <= 45)
    {
        result += 324;
    }
    else if (id > 45 && id == 46)
    {
        result += 325;
    }
    return result + id;
}

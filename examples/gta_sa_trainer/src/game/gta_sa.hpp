/**
 * @file gta_sa.hpp
 *
 * @brief Declaration of the GtaSA class. This class is inherited from the
 *        ExternalProcess class and provides functionality to interact with the
 *        game process.
 *
 */
#ifndef GTA_SA_HPP
#define GTA_SA_HPP

#include "../../../../src/external_process.hpp"
#include "../utils.hpp"
#include "types.hpp"

using namespace P1ExternalProcess;

class GtaSA : public ExternalProcess
{
public:
    GtaSA(void);
    uint32_t get_player_ped(void);
    vec3f get_ped_pos(uint32_t ped);
    float get_ped_health(uint32_t ped);
    uint8_t get_ped_armed_weapon(uint32_t ped);
    uint32_t get_ped_by_handle(uint32_t handle);
    vec2ui get_screen_resolution(void);
    vec2f get_crosshair_position(void);
    float get_aspect_ratio(void);
    float get_cam_x_angle(void);
    float get_cam_y_angle(void);
    uint32_t get_streamed_peds(uint32_t **out_ped_ptr_array);
    bool is_player_aiming(void);
    mat44f get_view_matrix(void);
    vec3f get_bone_position(uint32_t ped, uint32_t bone_id, uint8_t is_dynamic);
    vec3f get_bone_position_quick(uint32_t ped, uint32_t bone_id,
                                  uint8_t is_dynamic);
    vec3f get_screen_position(const vec3f &world_pos);
    void give_weapon(uint32_t ped, enWeaponId weapon_id, uint32_t ammo,
                     uint32_t like_unused);
    void show_dialog_message(const char *text, uint32_t time);
    void unlock_fps(bool state);
    void auto_sprint_bug(bool state);

private:
    uint16_t get_onfoot_key_state(enOnfootKeys id);
    void set_onfoot_key_state(enOnfootKeys id, uint16_t state);
    void request_model(int32_t id, uint32_t flags);
    void set_model_deletable(int32_t id);
    uint32_t get_weapon_model_id(enWeaponId id);

    uint32_t _ped_pool;
    uint32_t _ped_pool_objects;
    uint32_t _ped_pool_bytemap;
};

#endif /* GTA_SA_HPP */

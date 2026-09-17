#pragma once

#include <cstdint>
#include <string>
#include "../sdk.hpp"

namespace valve::entity {

    struct WeaponInfo {
        std::string formatted_name;
        std::string icon;
    };

    uintptr_t get_entity_by_index(int index);
    uintptr_t get_entity_by_handle(uint32_t handle);
    const char* get_schema_name(uintptr_t entity);

    uintptr_t get_local_player_controller();
    uintptr_t get_local_player_pawn();
    uint8_t get_team(uintptr_t entity);
    int get_health(uintptr_t entity);
    uint8_t get_life_state(uintptr_t entity);
    std::string get_player_name(uintptr_t controller);

    std::string get_active_weapon_name(uintptr_t pawn);
    WeaponInfo get_weapon_info(const std::string& raw_name);

    bool get_bone_position(uintptr_t pawn, uint32_t bone_id, vector3& out_pos);

}

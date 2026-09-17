
#include "legit.hpp"
#include "../keybinds/keybinds.hpp"
#include "../visuals/visuals.hpp"
#include "../../widgets/custom_widgets.hpp"
#include "../../gui/gui/gui.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"
#include "../../valve/schema/schema.hpp"
#include "../../valve/sdk.hpp"
#include "../../valve/entity/entity.hpp"
#include "../../valve/math/math.hpp"
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <cctype>
#include <chrono>

namespace features::legit {

    static int current_main_subtab = 0;
    static const char* const main_subtab_names[] = { "usp_silencer", "mp7", "nova", "ak47", "ssg08", "negev" };

    int get_active_weapon_category() {
        uintptr_t local_pawn = valve::entity::get_local_player_pawn();
        if (!local_pawn) return 0; 

        std::string raw_wep = valve::entity::get_active_weapon_name(local_pawn);
        if (raw_wep.empty()) return 0;

        uint32_t hash = fnv1a::runtime_hash(raw_wep.c_str());
        switch (hash) {
            
            case "deagle"_hash:
            case "glock"_hash:
            case "usp_silencer"_hash:
            case "hkp2000"_hash:
            case "p250"_hash:
            case "cz75a"_hash:
            case "fiveseven"_hash:
            case "tec9"_hash:
            case "revolver"_hash:
            case "elite"_hash:
                return 0;

            
            case "mac10"_hash:
            case "mp9"_hash:
            case "mp7"_hash:
            case "mp5sd"_hash:
            case "ump45"_hash:
            case "p90"_hash:
            case "bizon"_hash:
                return 1;

            
            case "nova"_hash:
            case "xm1014"_hash:
            case "sawedoff"_hash:
            case "mag7"_hash:
                return 2;

            
            case "ak47"_hash:
            case "m4a1"_hash:
            case "m4a1_silencer"_hash:
            case "galilar"_hash:
            case "famas"_hash:
            case "aug"_hash:
            case "sg556"_hash:
                return 3;

            
            case "awp"_hash:
            case "ssg08"_hash:
            case "scar20"_hash:
            case "g3sg1"_hash:
                return 4;

            
            case "m249"_hash:
            case "negev"_hash:
                return 5;

            default:
                break;
        }

        return 0;
    }

    void render() {
        widgets::subtab_bar(&current_main_subtab, main_subtab_names, IM_ARRAYSIZE(main_subtab_names));

        float full_w = ImGui::GetContentRegionAvail().x;
        float col_w = (full_w - 6.0f) * 0.5f;

        int active_idx = std::clamp(current_main_subtab, 0, 5);
        auto& active_cfg = cfg.weapons[active_idx];

        ImGui::BeginGroup();
        if (widgets::begin_section("Aim Assist", col_w, 0.0f)) {
            widgets::toggle_with_keybind("Enabled", &active_cfg.enabled, &active_cfg.key, &active_cfg.key_mode);
            static const char* const target_parts[] = { "Head", "Neck", "Chest", "Pelvis", "Chicken", "Closest" };
            widgets::dropdown("Target part", &active_cfg.target_part, target_parts, IM_ARRAYSIZE(target_parts));
            widgets::slider_float("Fov radius", &active_cfg.fov, 0.0f, 30.0f);
            widgets::slider_float("Smoothing", &active_cfg.smooth, 1.0f, 20.0f);
            widgets::toggle_with_color("Show Fov", &active_cfg.draw_fov, active_cfg.fov_color, "Fov Color");
            widgets::end_section();
        }
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 6.0f);

        ImGui::BeginGroup();
        if (widgets::begin_section("Triggerbot", col_w, 0.0f)) {
            widgets::toggle_with_keybind("Enabled", &active_cfg.trigger_enabled, &active_cfg.trigger_key, &active_cfg.trigger_key_mode);
            widgets::toggle("Only", &active_cfg.trigger_filter_enabled);
            if (active_cfg.trigger_filter_enabled) {
                static const char* const trigger_parts[] = { "Head", "Neck", "Chest", "Pelvis", "Closest" };
                widgets::dropdown("Target part", &active_cfg.trigger_target_part, trigger_parts, IM_ARRAYSIZE(trigger_parts));
            }
            widgets::slider_int("Delay (ms)", &active_cfg.trigger_delay, 0, 300);
            widgets::toggle("Seeded delay", &active_cfg.trigger_seeded_delay);
            widgets::end_section();
        }
        ImGui::EndGroup();
    }

    void draw_fov() {
        int cat_idx = get_active_weapon_category();
        const auto& active_cfg = cfg.weapons[cat_idx];

        if (!active_cfg.draw_fov || active_cfg.fov <= 0.0f) return;

        ImVec2 center = ImGui::GetIO().DisplaySize;
        center.x *= 0.5f;
        center.y *= 0.5f;

        float radius = (center.y / tanf((90.0f * 0.5f) * (3.14159265f / 180.0f))) * tanf((active_cfg.fov * 0.5f) * (3.14159265f / 180.0f));
        if (radius <= 0.0f) return;

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        uint8_t r = static_cast<uint8_t>(active_cfg.fov_color[0] * 255.0f);
        uint8_t g = static_cast<uint8_t>(active_cfg.fov_color[1] * 255.0f);
        uint8_t b = static_cast<uint8_t>(active_cfg.fov_color[2] * 255.0f);
        uint8_t a = static_cast<uint8_t>(active_cfg.fov_color[3] * 255.0f);
        ImU32 col = IM_COL32(r, g, b, a);

        draw_list->AddCircle(center, radius, col, 64, 1.0f);
    }

    void run() {
        int cat_idx = get_active_weapon_category();
        const auto& active_cfg = cfg.weapons[cat_idx];

        if (!active_cfg.enabled || !keybinds::is_active("Aim Assist")) return;

        uintptr_t local_pawn = valve::entity::get_local_player_pawn();
        if (!local_pawn) return;

        uint8_t local_team = valve::entity::get_team(local_pawn);
        bool is_ffa = features::visuals::is_ffa_mode();

        ImVec2 screen_center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        view_matrix_t view_matrix = memory::read<view_matrix_t>(interfaces::view_matrix_ptr);

        float max_fov_px = (screen_center.y / tanf((90.0f * 0.5f) * (3.14159265f / 180.0f))) * tanf((active_cfg.fov * 0.5f) * (3.14159265f / 180.0f));
        if (max_fov_px <= 0.0f) return;

        float best_dist = max_fov_px;
        vector2 best_target_screen{ 0.0f, 0.0f };
        bool found = false;

        static uint32_t off_hPlayerPawn = 0;
        static uint32_t off_pGameSceneNode = 0;
        static uint32_t off_vecAbsOrigin = 0;
        if (!off_hPlayerPawn) off_hPlayerPawn = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_hPlayerPawn"));
        if (!off_pGameSceneNode) off_pGameSceneNode = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pGameSceneNode"));
        if (!off_vecAbsOrigin) off_vecAbsOrigin = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_vecAbsOrigin"));

        bool is_chicken_mode = (active_cfg.target_part == 4);

        if (is_chicken_mode) {
            for (int i = 1; i < 2048; ++i) {
                uintptr_t entity = valve::entity::get_entity_by_index(i);
                if (!entity) continue;

                const char* class_name = valve::entity::get_schema_name(entity);
                if (!class_name) continue;

                std::string name_lower = class_name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                if (name_lower.find("chicken") == std::string::npos) continue;

                static const uint32_t chicken_bones[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
                bool bone_hit = false;

                for (uint32_t b_id : chicken_bones) {
                    vector3 bone_pos{};
                    if (valve::entity::get_bone_position(entity, b_id, bone_pos) && (bone_pos.x != 0.0f || bone_pos.y != 0.0f)) {
                        vector2 screen_pos{};
                        if (valve::math::world_to_screen(bone_pos, screen_pos, view_matrix, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y)) {
                            float dx = screen_pos.x - screen_center.x;
                            float dy = screen_pos.y - screen_center.y;
                            float dist = sqrtf(dx * dx + dy * dy);

                            if (dist < best_dist) {
                                best_dist = dist;
                                best_target_screen = screen_pos;
                                found = true;
                                bone_hit = true;
                            }
                        }
                    }
                }

                if (!bone_hit && off_pGameSceneNode && off_vecAbsOrigin) {
                    uintptr_t game_scene_node = memory::read<uintptr_t>(entity + off_pGameSceneNode);
                    if (game_scene_node) {
                        vector3 origin = memory::read<vector3>(game_scene_node + off_vecAbsOrigin);
                        if (origin.x != 0.0f || origin.y != 0.0f) {
                            origin.z += 6.0f;
                            vector2 screen_pos{};
                            if (valve::math::world_to_screen(origin, screen_pos, view_matrix, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y)) {
                                float dx = screen_pos.x - screen_center.x;
                                float dy = screen_pos.y - screen_center.y;
                                float dist = sqrtf(dx * dx + dy * dy);

                                if (dist < best_dist) {
                                    best_dist = dist;
                                    best_target_screen = screen_pos;
                                    found = true;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            static const uint32_t head_bone[] = { 7 };
            static const uint32_t neck_bone[] = { 6 };
            static const uint32_t chest_bone[] = { 4, 23 };
            static const uint32_t pelvis_bone[] = { 1, 2 };
            static const uint32_t all_bones[] = { 7, 6, 4, 23, 3, 1, 2 };

            const uint32_t* bones_ptr = all_bones;
            size_t bones_count = 7;

            if (active_cfg.target_part == 0) { bones_ptr = head_bone; bones_count = 1; }
            else if (active_cfg.target_part == 1) { bones_ptr = neck_bone; bones_count = 1; }
            else if (active_cfg.target_part == 2) { bones_ptr = chest_bone; bones_count = 2; }
            else if (active_cfg.target_part == 3) { bones_ptr = pelvis_bone; bones_count = 2; }

            for (int i = 1; i <= 64; ++i) {
                uintptr_t entity = valve::entity::get_entity_by_index(i);
                if (!entity) continue;

                const char* class_name = valve::entity::get_schema_name(entity);
                if (!class_name || strcmp(class_name, "CCSPlayerController") != 0) continue;

                uint32_t pawn_handle = memory::read<uint32_t>(entity + off_hPlayerPawn);
                uintptr_t pawn = valve::entity::get_entity_by_handle(pawn_handle);
                if (!pawn || pawn == local_pawn) continue;

                int health = valve::entity::get_health(pawn);
                uint8_t life_state = valve::entity::get_life_state(pawn);
                if (health <= 0 || life_state != 0) continue;

                uint8_t team = valve::entity::get_team(pawn);
                bool is_enemy = (is_ffa || (local_team != 0 ? (team != local_team) : true));
                if (!is_enemy) continue;

                for (size_t b = 0; b < bones_count; ++b) {
                    uint32_t bone_id = bones_ptr[b];
                    vector3 bone_pos{};
                    if (!valve::entity::get_bone_position(pawn, bone_id, bone_pos)) continue;

                    vector2 screen_pos{};
                    if (!valve::math::world_to_screen(bone_pos, screen_pos, view_matrix, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y)) continue;

                    float dx = screen_pos.x - screen_center.x;
                    float dy = screen_pos.y - screen_center.y;
                    float dist = sqrtf(dx * dx + dy * dy);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_target_screen = screen_pos;
                        found = true;
                    }
                }
            }
        }

        if (found) {
            float delta_x = best_target_screen.x - screen_center.x;
            float delta_y = best_target_screen.y - screen_center.y;

            float smooth_val = (active_cfg.smooth < 1.0f) ? 1.0f : active_cfg.smooth;
            float move_x = delta_x / smooth_val;
            float move_y = delta_y / smooth_val;

            if (fabsf(move_x) > 0.01f || fabsf(move_y) > 0.01f) {
                mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(move_x), static_cast<DWORD>(move_y), 0, 0);
            }
        }
    }

    void run_triggerbot() {
        int cat_idx = get_active_weapon_category();
        const auto& active_cfg = cfg.weapons[cat_idx];

        if (!active_cfg.trigger_enabled || !keybinds::is_active("Triggerbot")) return;

        uintptr_t local_pawn = valve::entity::get_local_player_pawn();
        if (!local_pawn) return;

        uint8_t local_team = valve::entity::get_team(local_pawn);
        bool is_ffa = features::visuals::is_ffa_mode();

        ImVec2 screen_center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        view_matrix_t view_matrix = memory::read<view_matrix_t>(interfaces::view_matrix_ptr);

        static uint32_t off_hPlayerPawn = 0;
        if (!off_hPlayerPawn) off_hPlayerPawn = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_hPlayerPawn"));

        static const uint32_t head_bone[] = { 7 };
        static const uint32_t neck_bone[] = { 6 };
        static const uint32_t chest_bone[] = { 4, 23 };
        static const uint32_t pelvis_bone[] = { 1, 2 };
        static const uint32_t all_bones[] = { 7, 6, 4, 23, 3, 1, 2 };

        const uint32_t* bones_ptr = all_bones;
        size_t bones_count = 7;

        if (active_cfg.trigger_filter_enabled) {
            if (active_cfg.trigger_target_part == 0) { bones_ptr = head_bone; bones_count = 1; }
            else if (active_cfg.trigger_target_part == 1) { bones_ptr = neck_bone; bones_count = 1; }
            else if (active_cfg.trigger_target_part == 2) { bones_ptr = chest_bone; bones_count = 2; }
            else if (active_cfg.trigger_target_part == 3) { bones_ptr = pelvis_bone; bones_count = 2; }
        }

        bool target_under_crosshair = false;
        const float hit_threshold_px = 18.0f;

        static uint32_t off_pGameSceneNode = 0;
        static uint32_t off_vecAbsOrigin = 0;
        if (!off_pGameSceneNode) off_pGameSceneNode = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pGameSceneNode"));
        if (!off_vecAbsOrigin) off_vecAbsOrigin = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_vecAbsOrigin"));

        for (int i = 1; i <= 64; ++i) {
            uintptr_t entity = valve::entity::get_entity_by_index(i);
            if (!entity) continue;

            const char* class_name = valve::entity::get_schema_name(entity);
            if (!class_name || strcmp(class_name, "CCSPlayerController") != 0) continue;

            uint32_t pawn_handle = memory::read<uint32_t>(entity + off_hPlayerPawn);
            uintptr_t pawn = valve::entity::get_entity_by_handle(pawn_handle);
            if (!pawn || pawn == local_pawn) continue;

            int health = valve::entity::get_health(pawn);
            uint8_t life_state = valve::entity::get_life_state(pawn);
            if (health <= 0 || life_state != 0) continue;

            uint8_t team = valve::entity::get_team(pawn);
            bool is_enemy = (is_ffa || (local_team != 0 ? (team != local_team) : true));
            if (!is_enemy) continue;

            for (size_t b = 0; b < bones_count; ++b) {
                uint32_t bone_id = bones_ptr[b];
                vector3 bone_pos{};
                if (valve::entity::get_bone_position(pawn, bone_id, bone_pos)) {
                    vector2 screen_pos{};
                    if (valve::math::world_to_screen(bone_pos, screen_pos, view_matrix, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y)) {
                        float dx = screen_pos.x - screen_center.x;
                        float dy = screen_pos.y - screen_center.y;
                        float dist = sqrtf(dx * dx + dy * dy);

                        if (dist <= hit_threshold_px) {
                            target_under_crosshair = true;
                            break;
                        }
                    }
                }
            }

            if (!target_under_crosshair && off_pGameSceneNode && off_vecAbsOrigin) {
                uintptr_t scene_node = memory::read<uintptr_t>(pawn + off_pGameSceneNode);
                if (scene_node) {
                    vector3 origin = memory::read<vector3>(scene_node + off_vecAbsOrigin);
                    origin.z += 40.0f;
                    vector2 screen_pos{};
                    if (valve::math::world_to_screen(origin, screen_pos, view_matrix, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y)) {
                        float dx = screen_pos.x - screen_center.x;
                        float dy = screen_pos.y - screen_center.y;
                        float dist = sqrtf(dx * dx + dy * dy);

                        if (dist <= 22.0f) {
                            target_under_crosshair = true;
                        }
                    }
                }
            }

            if (target_under_crosshair) break;
        }

        static bool is_waiting = false;
        static auto start_time = std::chrono::steady_clock::now();
        static int current_delay_ms = 0;

        if (target_under_crosshair) {
            if (!is_waiting) {
                is_waiting = true;
                start_time = std::chrono::steady_clock::now();
                int base_delay = (std::max)(0, active_cfg.trigger_delay);
                int seed_offset = 0;
                if (active_cfg.trigger_seeded_delay && base_delay > 0) {
                    seed_offset = (rand() % 19) - 9;
                } else if (active_cfg.trigger_seeded_delay) {
                    seed_offset = rand() % 15;
                }
                current_delay_ms = (std::max)(0, base_delay + seed_offset);
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
            if (elapsed >= current_delay_ms) {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                is_waiting = false;
            }
        } else {
            is_waiting = false;
        }
    }

}

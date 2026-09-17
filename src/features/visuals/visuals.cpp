
#include "visuals.hpp"
#include "../legit/legit.hpp"
#include "../../widgets/custom_widgets.hpp"
#include "../../gui/theme/theme.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"
#include "../../valve/schema/schema.hpp"
#include "../../valve/sdk.hpp"
#include "../../valve/entity/entity.hpp"
#include "../../valve/math/math.hpp"
#include "../../valve/icons/icons.hpp"
#include "../../imgui/imgui.h"
#include <algorithm>
#include <cmath>

namespace features::visuals {

    static int current_subtab = 0;
    static const char* subtab_names[] = { "Enemy", "Team", "Local" };

    static ImU32 to_imcol(const float col[4]) {
        return IM_COL32(
            static_cast<int>(col[0] * 255.0f),
            static_cast<int>(col[1] * 255.0f),
            static_cast<int>(col[2] * 255.0f),
            static_cast<int>(col[3] * 255.0f)
        );
    }

    static const char* material_names[] = {
        "White", "Latex", "Glow", "Ghost", "Flat", "Glow2", "Glass", "Generic",
        "Unlit", "Solid", "Wireframe", "Bloom", "Illuminate", "Gost", "Crystal", "Gost2",
        "Metallic", "Flow", "Dark Matter", "Data", "Chrome", "Plastic", "Energy", "Hologram",
        "Galaxy", "Gold", "Neon", "Xray", "Liquid", "Pearl", "Distortion", "Outlines"
    };

    static void render_esp_settings(EspTargetSettings& esp) {
        widgets::toggle_with_color("Bounding Boxes", &esp.box, esp.box_color);
        widgets::toggle_with_color("Name", &esp.name, esp.name_color);

        widgets::toggle_with_settings("Health Bar", &esp.health, "Health Options", [&]() {
            widgets::toggle_with_color("Gradient", &esp.health_gradient, esp.health_gradient_color);
            widgets::toggle_with_color("Outline", &esp.health_outline, esp.health_outline_color);
            widgets::toggle_with_color("Background", &esp.health_background, esp.health_bg_color);
        }, 170.0f);

        static const char* weapon_display_items[] = { "Text", "Icon", "Text + Icon" };
        widgets::toggle_with_settings("Weapon ESP", &esp.weapon, "Weapon Options", [&]() {
            widgets::dropdown("Display Mode", &esp.weapon_display, weapon_display_items, IM_ARRAYSIZE(weapon_display_items));
            widgets::toggle_with_color("Weapon Color", &esp.weapon, esp.weapon_color, "Weapon Color");
        }, 170.0f);

        widgets::toggle_with_color("Skeleton", &esp.skeleton, esp.skeleton_color);
        if (esp.skeleton) {
            widgets::slider_float("Thickness", &esp.skeleton_thickness, 1.0f, 4.0f);
        }
    }

    static void render_chams_settings(const char* label, ChamsTargetSettings& chams) {
        widgets::toggle_with_settings(label, &chams.enabled, label, [&]() {
            widgets::dropdown("Material", &chams.material, material_names, IM_ARRAYSIZE(material_names));
            widgets::toggle_with_color("Visible Color", &chams.enabled, chams.visible_color, "Visible Color");
            widgets::toggle_with_color("Occluded", &chams.occluded, chams.occluded_color, "Occluded Color");
            
        }, 220.0f);
    }

    void render() {
        float full_w = ImGui::GetContentRegionAvail().x;
        float col_w = (full_w - 6.0f) * 0.5f;

        ImGui::BeginGroup();
        if (widgets::begin_section_subtabs(&current_subtab, subtab_names, IM_ARRAYSIZE(subtab_names), col_w)) {
            if (current_subtab == 0) { 
                render_esp_settings(cfg.enemy);
                render_chams_settings("Chams", cfg.chams_enemy);
                render_chams_settings("Ragdoll Chams", cfg.chams_ragdoll_enemy);
            } else if (current_subtab == 1) { 
                render_esp_settings(cfg.team);
                render_chams_settings("Chams", cfg.chams_team);
                render_chams_settings("Ragdoll Chams", cfg.chams_ragdoll_team);
            } else if (current_subtab == 2) { 
                render_chams_settings("Player Chams", cfg.chams_local);
                render_chams_settings("Ragdoll Chams", cfg.chams_ragdoll_local);
                render_chams_settings("Arms Chams", cfg.chams_arms);
                render_chams_settings("Weapon Chams", cfg.chams_weapon);
            }
            widgets::end_section();
        }
        ImGui::EndGroup();
    }

    static bool check_is_deathmatch() {
        return false;
    }

    bool is_ffa_mode() {
        return check_is_deathmatch();
    }

    static constexpr std::pair<uint32_t, uint32_t> k_skeleton_links[] = {
        { 1, 3 },   
        { 3, 4 },   
        { 4, 23 },  
        { 23, 6 },  
        { 6, 7 },   
        { 6, 9 },   
        { 9, 10 },  
        { 10, 11 }, 
        { 6, 13 },  
        { 13, 14 }, 
        { 14, 15 }, 
        { 1, 17 },  
        { 17, 18 }, 
        { 18, 19 }, 
        { 1, 20 },  
        { 20, 21 }, 
        { 21, 22 }  
    };

    static void draw_skeleton(ImDrawList* draw_list, uintptr_t pawn, const view_matrix_t& view_matrix, float screen_w, float screen_h, const float col[4], float thickness) {

        ImU32 color = to_imcol(col);
        float line_thickness = thickness > 0.1f ? thickness : 1.0f;

        for (const auto& [a, b] : k_skeleton_links) {
            vector3 p1_3d, p2_3d;
            if (!valve::entity::get_bone_position(pawn, a, p1_3d) ||
                !valve::entity::get_bone_position(pawn, b, p2_3d)) {
                continue;
            }

            float dx = p1_3d.x - p2_3d.x;
            float dy = p1_3d.y - p2_3d.y;
            float dz = p1_3d.z - p2_3d.z;
            if (dx * dx + dy * dy + dz * dz > (72.0f * 72.0f)) {
                continue;
            }

            vector2 p1_2d, p2_2d;
            if (valve::math::world_to_screen(p1_3d, p1_2d, view_matrix, screen_w, screen_h) &&
                valve::math::world_to_screen(p2_3d, p2_2d, view_matrix, screen_w, screen_h)) {

                float sx = p1_2d.x - p2_2d.x;
                float sy = p1_2d.y - p2_2d.y;
                if (sx * sx + sy * sy > 250000.0f) {
                    continue;
                }

                draw_list->AddLine(
                    ImVec2(p1_2d.x, p1_2d.y),
                    ImVec2(p2_2d.x, p2_2d.y),
                    color,
                    line_thickness
                );
            }
        }
    }

    static void draw_esp_internal() {
        if (!interfaces::view_matrix_ptr) return;

        uintptr_t local_pawn = valve::entity::get_local_player_pawn();
        if (!local_pawn) return;

        static uint32_t off_hPlayerPawn = 0;
        static uint32_t off_iHealth = 0;
        static uint32_t off_lifeState = 0;
        static uint32_t off_iTeamNum = 0;
        static uint32_t off_pGameSceneNode = 0;
        static uint32_t off_vecAbsOrigin = 0;
        static uint32_t off_pCollision = 0;
        static uint32_t off_vecMins = 0;
        static uint32_t off_vecMaxs = 0;

        if (!off_hPlayerPawn) off_hPlayerPawn = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_hPlayerPawn"));
        if (!off_iHealth) off_iHealth = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_iHealth"));
        if (!off_lifeState) off_lifeState = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_lifeState"));
        if (!off_iTeamNum) off_iTeamNum = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_iTeamNum"));
        if (!off_pGameSceneNode) off_pGameSceneNode = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pGameSceneNode"));
        if (!off_vecAbsOrigin) off_vecAbsOrigin = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_vecAbsOrigin"));
        if (!off_pCollision) off_pCollision = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pCollision"));
        if (!off_vecMins) off_vecMins = schema::lookup("CCollisionProperty", fnv1a::runtime_hash("m_vecMins"));
        if (!off_vecMaxs) off_vecMaxs = schema::lookup("CCollisionProperty", fnv1a::runtime_hash("m_vecMaxs"));

        if (!off_hPlayerPawn || !off_iHealth || !off_pGameSceneNode || !off_vecAbsOrigin) return;

        uint8_t local_team = memory::read<uint8_t>(local_pawn + off_iTeamNum);
        bool is_ffa = check_is_deathmatch();

        view_matrix_t view_matrix = memory::read<view_matrix_t>(interfaces::view_matrix_ptr);

        ImGuiIO& io = ImGui::GetIO();
        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;

        if (screen_w <= 0.0f || screen_h <= 0.0f) return;

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

        features::legit::draw_fov();
        features::legit::run();
        features::legit::run_triggerbot();

        int players_found = 0;
        int players_rendered = 0;

        static unsigned frame_counter = 0;
        bool do_log = ((++frame_counter) % 300 == 0);

        for (int i = 1; i <= 64; ++i) {
            uintptr_t entity = valve::entity::get_entity_by_index(i);
            if (!entity) continue;

            const char* class_name = valve::entity::get_schema_name(entity);
            if (!class_name) continue;

            if (fnv1a::runtime_hash(class_name) != fnv1a::runtime_hash("CCSPlayerController")) continue;

            uint32_t pawn_handle = memory::read<uint32_t>(entity + off_hPlayerPawn);
            uintptr_t pawn = valve::entity::get_entity_by_handle(pawn_handle);
            if (!pawn) continue;

            players_found++;

            uint8_t life_state = memory::read<uint8_t>(pawn + off_lifeState);
            int health = memory::read<int>(pawn + off_iHealth);
            if (life_state != 0 || health <= 0 || health > 100) continue;

            uint8_t team = memory::read<uint8_t>(pawn + off_iTeamNum);
            bool is_local = (pawn == local_pawn);
            bool is_enemy = !is_local && (is_ffa || (local_team != 0 ? (team != local_team) : true));

            if (is_local) continue;

            const EspTargetSettings* esp_cfg = is_enemy ? &cfg.enemy : &cfg.team;

            if (!esp_cfg->box && !esp_cfg->name && !esp_cfg->health && !esp_cfg->weapon && !esp_cfg->skeleton) continue;

            uintptr_t scene_node = memory::read<uintptr_t>(pawn + off_pGameSceneNode);
            if (!scene_node) continue;

            vector3 origin = memory::read<vector3>(scene_node + off_vecAbsOrigin);

            uintptr_t collision = memory::read<uintptr_t>(pawn + off_pCollision);
            vector3 mins = { -16.0f, -16.0f, 0.0f };
            vector3 maxs = { 16.0f, 16.0f, 72.0f };
            if (collision) {
                mins = memory::read<vector3>(collision + off_vecMins);
                maxs = memory::read<vector3>(collision + off_vecMaxs);
            }

            float min_x, min_y, max_x, max_y;
            bool valid = false;
            valve::math::calculate_bbox(origin, mins, maxs, view_matrix, screen_w, screen_h, min_x, min_y, max_x, max_y, valid);

            if (esp_cfg->skeleton) {
                draw_skeleton(draw_list, pawn, view_matrix, screen_w, screen_h, esp_cfg->skeleton_color, esp_cfg->skeleton_thickness);
            }

            if (!valid) continue;

            float box_w = max_x - min_x;
            float box_h = max_y - min_y;
            if (box_w <= 0.0f || box_h <= 0.0f) continue;

            players_rendered++;

            if (esp_cfg->box) {
                draw_list->AddRect(ImVec2(min_x, min_y), ImVec2(max_x, max_y), to_imcol(esp_cfg->box_color));
            }

            if (esp_cfg->name) {
                std::string player_name = valve::entity::get_player_name(entity);
                if (!player_name.empty()) {
                    ImVec2 txt_sz = ImGui::CalcTextSize(player_name.c_str());
                    float text_x = std::floor(min_x + (box_w - txt_sz.x) * 0.5f);
                    float text_y = std::floor(min_y - txt_sz.y - 2.0f);

                    ImU32 name_col = to_imcol(esp_cfg->name_color);
                    gui::theme::draw_text_stroke(draw_list, player_name.c_str(), ImVec2(text_x, text_y), name_col);
                }
            }

            if (esp_cfg->health) {
                float bar_w = 2.0f;
                float bar_pad = 4.0f;
                float bar_x = min_x - bar_pad - bar_w;
                float hp_frac = (std::clamp)(static_cast<float>(health) / 100.0f, 0.0f, 1.0f);
                float fill_h = (std::clamp)(box_h * hp_frac, 0.0f, box_h);

                ImVec2 rect_min(bar_x - 1.0f, min_y - 1.0f);
                ImVec2 rect_max(bar_x + bar_w + 1.0f, max_y + 1.0f);

                if (esp_cfg->health_background) {
                    draw_list->AddRectFilled(rect_min, rect_max, to_imcol(esp_cfg->health_bg_color));
                }

                if (fill_h > 0.0f) {
                    ImU32 hp_col_bottom = IM_COL32(static_cast<int>((1.0f - hp_frac) * 255.0f), static_cast<int>(hp_frac * 255.0f), 0, 255);
                    ImU32 hp_col_top = esp_cfg->health_gradient ? to_imcol(esp_cfg->health_gradient_color) : hp_col_bottom;

                    float top_y = (std::max)(min_y, max_y - fill_h);
                    draw_list->AddRectFilledMultiColor(
                        ImVec2(bar_x, top_y),
                        ImVec2(bar_x + bar_w, max_y),
                        hp_col_top,
                        hp_col_top,
                        hp_col_bottom,
                        hp_col_bottom
                    );
                }

                if (esp_cfg->health_outline) {
                    draw_list->AddRect(rect_min, rect_max, to_imcol(esp_cfg->health_outline_color));
                }
            }

            if (esp_cfg->weapon) {
                std::string raw_wep = valve::entity::get_active_weapon_name(pawn);
                if (!raw_wep.empty()) {
                    auto wep_info = valve::entity::get_weapon_info(raw_wep);
                    
                    bool show_icon = (esp_cfg->weapon_display == 1 || esp_cfg->weapon_display == 2);
                    bool show_text = (esp_cfg->weapon_display == 0 || esp_cfg->weapon_display == 2);

                    float total_h = 0.0f;
                    float cur_y = max_y + 4.0f;

                    if (show_icon) {
                        std::string icon_key = (raw_wep == "knife_ct" || raw_wep == "knife_t" || raw_wep.find("knife") != std::string::npos || raw_wep.find("bayonet") != std::string::npos) ? "knife" : raw_wep;
                        auto* icon_data = valve::icons::get(icon_key, 0.70f);
                        if (icon_data && icon_data->texture) {
                            float iw = icon_data->width;
                            float ih = icon_data->height;
                            float ix = std::floor(min_x + (box_w - iw) * 0.5f);
                            float iy = std::floor(cur_y + total_h);

                            ImU32 outline_col = IM_COL32(0, 0, 0, 255);
                            ImU32 icon_col = to_imcol(esp_cfg->weapon_color);

                            draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix - 1.0f, iy), ImVec2(ix + iw - 1.0f, iy + ih), ImVec2(0, 0), ImVec2(1, 1), outline_col);
                            draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix + 1.0f, iy), ImVec2(ix + iw + 1.0f, iy + ih), ImVec2(0, 0), ImVec2(1, 1), outline_col);
                            draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix, iy - 1.0f), ImVec2(ix + iw, iy + ih - 1.0f), ImVec2(0, 0), ImVec2(1, 1), outline_col);
                            draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix, iy + 1.0f), ImVec2(ix + iw, iy + ih + 1.0f), ImVec2(0, 0), ImVec2(1, 1), outline_col);

                            draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImVec2(0, 0), ImVec2(1, 1), icon_col);

                            total_h += ih + 2.0f;
                        } else {
                            show_text = true;
                        }
                    }

                    if (show_text) {
                        std::string label_str = wep_info.formatted_name;
                        if (!label_str.empty()) {
                            ImVec2 txt_sz = ImGui::CalcTextSize(label_str.c_str());
                            float text_x = std::floor(min_x + (box_w - txt_sz.x) * 0.5f);
                            float text_y = std::floor(cur_y + total_h);

                            ImU32 wep_col = to_imcol(esp_cfg->weapon_color);
                            gui::theme::draw_text_stroke(draw_list, label_str.c_str(), ImVec2(text_x, text_y), wep_col);
                        }
                    }
                }
            }
        }

        (void)do_log;
        (void)players_found;
        (void)players_rendered;
    }

    void draw_esp() {
        try {
            draw_esp_internal();
        }
        catch (...) {
        }
    }

}

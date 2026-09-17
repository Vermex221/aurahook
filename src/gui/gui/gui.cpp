
#include "gui.hpp"
#include "../theme/theme.hpp"
#include "../../imgui/imgui_internal.h"
#include "../../widgets/custom_widgets.hpp"
#include "../../features/legit/legit.hpp"
#include "../../features/rage/rage.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/skins/skins.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/settings/settings.hpp"
#include "../../features/keybinds/keybinds.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"
#include "../../core/window/window.hpp"
#include "../../valve/schema/schema.hpp"
#include "../../valve/entity/entity.hpp"
#include "../../valve/steam/steam_avatar.hpp"
#include <windows.h>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cmath>

namespace gui {

    namespace notifications {
        static std::vector<Notification> notifs;

        void push(const char* message, Type type, float duration) {
            Notification n;
            n.title = "";
            n.message = message ? message : "";
            n.type = type;
            n.duration = duration;
            n.max_duration = duration;
            n.current_x = -200.0f;
            n.alpha = 0.0f;
            notifs.push_back(n);
        }

        void push(const char* title, const char* message, Type type, float duration) {
            Notification n;
            n.title = title ? title : "";
            n.message = message ? message : "";
            n.type = type;
            n.duration = duration;
            n.max_duration = duration;
            n.current_x = -200.0f;
            n.alpha = 0.0f;
            notifs.push_back(n);
        }

        void render() {
            float delta_time = ImGui::GetIO().DeltaTime;
            float start_y = 50.0f;
            const float target_x = 20.0f;

            for (size_t i = 0; i < notifs.size();) {
                Notification& n = notifs[i];
                n.duration -= delta_time;

                if (n.duration <= 0.0f) {
                    notifs.erase(notifs.begin() + i);
                    continue;
                }

                float elapsed = n.max_duration - n.duration;
                if (elapsed < 0.25f) {
                    float t = elapsed / 0.25f;
                    n.alpha = t;
                    n.current_x = -150.0f + (target_x + 150.0f) * (1.0f - (1.0f - t) * (1.0f - t));
                } else if (n.duration < 0.25f) {
                    float t = n.duration / 0.25f;
                    n.alpha = t;
                    n.current_x = target_x - (1.0f - t) * 60.0f;
                } else {
                    n.alpha = 1.0f;
                    n.current_x = target_x;
                }
                n.alpha = std::clamp(n.alpha, 0.0f, 1.0f);

                ImU32 notif_accent = IM_COL32((int)(theme::accent_color.x * 255), (int)(theme::accent_color.y * 255), (int)(theme::accent_color.z * 255), (int)(n.alpha * 255));

                char notif_text[256];
                if (!n.title.empty()) {
                    snprintf(notif_text, sizeof(notif_text), "%s: %s", n.title.c_str(), n.message.c_str());
                } else {
                    snprintf(notif_text, sizeof(notif_text), "%s", n.message.c_str());
                }

                ImVec2 text_sz = ImGui::CalcTextSize(notif_text);
                float width = text_sz.x + 18.0f;
                float height = 22.0f;

                ImVec2 pos = ImVec2(n.current_x, start_y);
                ImVec2 max = ImVec2(pos.x + width, pos.y + height);

                ImDrawList* draw_list = ImGui::GetForegroundDrawList();

                draw_list->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(max.x + 1, max.y + 1), IM_COL32(0, 0, 0, (int)(n.alpha * 255)));
                draw_list->AddRect(pos, max, IM_COL32(52, 52, 52, (int)(n.alpha * 255)));
                draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, max.y - 1), IM_COL32(35, 35, 35, (int)(n.alpha * 255)));
                draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + 2, max.y - 1), notif_accent);

                float progress = std::clamp(n.duration / n.max_duration, 0.0f, 1.0f);
                float line_w = (width - 3.0f) * progress;
                if (line_w > 0.0f) {
                    draw_list->AddLine(
                        ImVec2(pos.x + 2, max.y - 1),
                        ImVec2(pos.x + 2 + line_w, max.y - 1),
                        notif_accent,
                        1.0f
                    );
                }

                float text_y = pos.y + (height - text_sz.y) * 0.5f - 0.5f;
                theme::draw_text_stroke(draw_list, notif_text, ImVec2(pos.x + 7, text_y), IM_COL32(239, 239, 239, (int)(n.alpha * 255)));

                start_y += height + 10.0f;
                i++;
            }
        }
    }

    void init() {
        ImGui::GetIO().ConfigDebugHighlightIdConflicts = false;
        theme::apply();
        features::keybinds::init();
        features::settings::init_configs();
        notifications::push("aurahook loaded!");
    }

    void render_watermark() {
        if (!features::settings::cfg.watermark)
            return;

        static ImVec2 wm_pos = ImVec2(20.0f, 12.0f);
        static bool dragging_wm = false;
        static ImVec2 drag_wm_offset = ImVec2(0, 0);

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm* timeinfo = std::localtime(&now_c);

        char date_str[64];
        if (timeinfo) {
            char month_abbr[16];
            std::strftime(month_abbr, sizeof(month_abbr), "%b", timeinfo);
            snprintf(date_str, sizeof(date_str), "%s. %d, %d", month_abbr, timeinfo->tm_mday, 1900 + timeinfo->tm_year);
        } else {
            snprintf(date_str, sizeof(date_str), "Sep. 15, 2026");
        }

        const char* menu_title = (features::settings::cfg.custom_menu_name && features::settings::cfg.menu_name[0] != '\0') ? features::settings::cfg.menu_name : "AuraHook";
        char watermark_text[128];
        snprintf(watermark_text, sizeof(watermark_text), "%s | Private | %s", menu_title, date_str);

        ImVec2 text_size = ImGui::CalcTextSize(watermark_text);
        float padding = 7.0f;
        float width = text_size.x + padding * 2.0f;
        float height = 20.0f;
        ImVec2 pos = wm_pos;
        ImVec2 max = ImVec2(pos.x + width, pos.y + height);

        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        ImRect wm_bb(pos, max);
        if (ImGui::IsMouseClicked(0) && wm_bb.Contains(mouse_pos)) {
            dragging_wm = true;
            drag_wm_offset = ImVec2(mouse_pos.x - wm_pos.x, mouse_pos.y - wm_pos.y);
        }
        if (dragging_wm) {
            if (ImGui::IsMouseDown(0)) {
                wm_pos = ImVec2(mouse_pos.x - drag_wm_offset.x, mouse_pos.y - drag_wm_offset.y);
                pos = wm_pos;
                max = ImVec2(pos.x + width, pos.y + height);
            } else {
                dragging_wm = false;
            }
        }

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        draw_list->AddRect(pos, max, ImGui::GetColorU32(theme::outline_color));
        draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, max.y - 1), ImGui::GetColorU32(theme::background_color));
        theme::draw_accent_bar(draw_list, ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, pos.y + 3), ImGui::GetColorU32(theme::accent_color));
        draw_list->AddLine(ImVec2(pos.x, pos.y + 3), ImVec2(max.x, pos.y + 3), ImGui::GetColorU32(theme::outline_color));

        float text_y = pos.y + (height - text_size.y) * 0.5f + 1.0f;
        theme::draw_text_stroke(draw_list, watermark_text, ImVec2(pos.x + padding, text_y), ImGui::GetColorU32(theme::text_color));
    }

    void render_activity_window() {
        if (!features::settings::cfg.keybind_list)
            return;

        static ImVec2 act_pos = ImVec2(20.0f, 660.0f);
        static bool dragging_act = false;
        static ImVec2 drag_act_offset = ImVec2(0, 0);

        auto active_binds = features::keybinds::get_active_binds();

        float width = 165.0f;
        for (const auto& b : active_binds) {
            char row_buf[128];
            const char* key_str = widgets::get_key_name(b.key);
            snprintf(row_buf, sizeof(row_buf), "[%s]: %s", key_str, b.name.c_str());
            float row_w = ImGui::CalcTextSize(row_buf).x + 16.0f;
            if (row_w > width) width = row_w;
        }

        float header_height = 20.0f;
        float row_height = 16.0f;
        float body_height = active_binds.empty() ? 4.0f : (active_binds.size() * row_height + 6.0f);
        float total_height = header_height + body_height;

        ImVec2 pos = act_pos;
        ImVec2 max = ImVec2(pos.x + width, pos.y + total_height);
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;

        ImRect header_bb(pos, ImVec2(max.x, pos.y + header_height));
        if (ImGui::IsMouseClicked(0) && header_bb.Contains(mouse_pos)) {
            dragging_act = true;
            drag_act_offset = ImVec2(mouse_pos.x - act_pos.x, mouse_pos.y - act_pos.y);
        }
        if (dragging_act) {
            if (ImGui::IsMouseDown(0)) {
                act_pos = ImVec2(mouse_pos.x - drag_act_offset.x, mouse_pos.y - drag_act_offset.y);
                pos = act_pos;
                max = ImVec2(pos.x + width, pos.y + total_height);
            } else {
                dragging_act = false;
            }
        }

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        draw_list->AddRect(pos, max, ImGui::GetColorU32(theme::outline_color));
        draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + header_height), ImVec2(max.x - 1, max.y - 1), ImGui::GetColorU32(theme::background_color));
        theme::draw_accent_bar(draw_list, ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, pos.y + 3), ImGui::GetColorU32(theme::accent_color));
        draw_list->AddLine(ImVec2(pos.x, pos.y + 3), ImVec2(max.x, pos.y + 3), ImGui::GetColorU32(theme::outline_color));
        theme::draw_gradient_v(
            draw_list,
            ImVec2(pos.x + 1, pos.y + 4),
            ImVec2(max.x - 1, pos.y + header_height),
            ImGui::GetColorU32(theme::inline_color),
            ImGui::GetColorU32(theme::gradient_color)
        );
        draw_list->AddLine(ImVec2(pos.x, pos.y + header_height), ImVec2(max.x, pos.y + header_height), ImGui::GetColorU32(theme::outline_color));
        theme::draw_text_stroke(draw_list, "Keybinds", ImVec2(pos.x + 6, pos.y + 4), ImGui::GetColorU32(theme::text_color));

        float cur_y = pos.y + header_height + 4.0f;
        if (!active_binds.empty()) {
            for (const auto& b : active_binds) {
                char row_buf[128];
                const char* key_str = widgets::get_key_name(b.key);
                snprintf(row_buf, sizeof(row_buf), "[%s]: %s", key_str, b.name.c_str());
                theme::draw_text_stroke(draw_list, row_buf, ImVec2(pos.x + 6, cur_y), ImGui::GetColorU32(theme::text_color));
                cur_y += row_height;
            }
        }
    }

    void render_spectator_window() {
        if (!features::misc::cfg.spectator_list)
            return;

        static ImVec2 spec_pos = ImVec2(20.0f, 510.0f);
        static bool dragging_spec = false;
        static ImVec2 drag_spec_offset = ImVec2(0, 0);

        auto specs = features::misc::get_spectators();

        float width = 165.0f;
        for (const auto& s : specs) {
            float name_w = ImGui::CalcTextSize(s.name.c_str()).x + 28.0f;
            if (name_w > width) width = name_w;
        }

        float header_height = 20.0f;
        float row_height = 18.0f;
        size_t spec_count = specs.size();
        float body_height = specs.empty() ? 4.0f : (spec_count * row_height + 6.0f);
        float total_height = header_height + body_height;

        ImVec2 pos = spec_pos;
        ImVec2 max = ImVec2(pos.x + width, pos.y + total_height);
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;

        ImRect header_bb(pos, ImVec2(max.x, pos.y + header_height));
        if (ImGui::IsMouseClicked(0) && header_bb.Contains(mouse_pos)) {
            dragging_spec = true;
            drag_spec_offset = ImVec2(mouse_pos.x - spec_pos.x, mouse_pos.y - spec_pos.y);
        }
        if (dragging_spec) {
            if (ImGui::IsMouseDown(0)) {
                spec_pos = ImVec2(mouse_pos.x - drag_spec_offset.x, mouse_pos.y - drag_spec_offset.y);
                pos = spec_pos;
                max = ImVec2(pos.x + width, pos.y + total_height);
            } else {
                dragging_spec = false;
            }
        }

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        draw_list->AddRect(pos, max, ImGui::GetColorU32(theme::outline_color));
        draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + header_height), ImVec2(max.x - 1, max.y - 1), ImGui::GetColorU32(theme::background_color));
        theme::draw_accent_bar(draw_list, ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, pos.y + 3), ImGui::GetColorU32(theme::accent_color));
        draw_list->AddLine(ImVec2(pos.x, pos.y + 3), ImVec2(max.x, pos.y + 3), ImGui::GetColorU32(theme::outline_color));
        theme::draw_gradient_v(
            draw_list,
            ImVec2(pos.x + 1, pos.y + 4),
            ImVec2(max.x - 1, pos.y + header_height),
            ImGui::GetColorU32(theme::inline_color),
            ImGui::GetColorU32(theme::gradient_color)
        );
        draw_list->AddLine(ImVec2(pos.x, pos.y + header_height), ImVec2(max.x, pos.y + header_height), ImGui::GetColorU32(theme::outline_color));
        theme::draw_text_stroke(draw_list, "Spectators", ImVec2(pos.x + 6, pos.y + 4), ImGui::GetColorU32(theme::text_color));

        float cur_y = pos.y + header_height + 4.0f;
        for (const auto& s : specs) {
            
            ImVec2 avatar_min = ImVec2(pos.x + 6.0f, cur_y + 1.0f);
            ImVec2 avatar_max = ImVec2(avatar_min.x + 14.0f, avatar_min.y + 14.0f);

            ID3D11ShaderResourceView* avatar_tex = valve::steam::get_avatar_texture(s.steam_id);
            if (avatar_tex) {
                draw_list->AddImage(reinterpret_cast<ImTextureID>(avatar_tex), avatar_min, avatar_max);
                draw_list->AddRect(avatar_min, avatar_max, ImGui::GetColorU32(theme::outline_color));
            } else {
                draw_list->AddRectFilled(avatar_min, avatar_max, IM_COL32(45, 45, 45, 255));
                draw_list->AddRect(avatar_min, avatar_max, ImGui::GetColorU32(theme::outline_color));

                
                draw_list->AddRectFilled(ImVec2(avatar_min.x + 4.0f, avatar_min.y + 3.0f), ImVec2(avatar_min.x + 10.0f, avatar_min.y + 7.0f), ImGui::GetColorU32(theme::accent_color));
                draw_list->AddRectFilled(ImVec2(avatar_min.x + 2.0f, avatar_min.y + 8.0f), ImVec2(avatar_min.x + 12.0f, avatar_min.y + 13.0f), ImGui::GetColorU32(theme::accent_color));
            }

            theme::draw_text_stroke(draw_list, s.name.c_str(), ImVec2(pos.x + 25.0f, cur_y + 1.0f), ImGui::GetColorU32(theme::text_color));
            cur_y += row_height;
        }
    }

    void render_bomb_info_window() {
        if (!features::misc::cfg.bomb_info)
            return;

        static ImVec2 bomb_pos = ImVec2(195.0f, 660.0f);
        static bool dragging_bomb = false;
        static ImVec2 drag_bomb_offset = ImVec2(0, 0);

        uintptr_t planted_c4_ptr = memory::read<uintptr_t>(interfaces::planted_c4);
        uintptr_t global_vars_ptr = memory::read<uintptr_t>(interfaces::global_vars);

        bool is_planted = false;
        float time_left = 0.0f;
        float timer_length = 40.0f;
        int bomb_site = 0;
        bool being_defused = false;
        int damage = 0;

        if (planted_c4_ptr && global_vars_ptr) {
            static uint32_t off_bBeingDefused = 0;
            static uint32_t off_bBombDefused = 0;
            static uint32_t off_bHasExploded = 0;
            static uint32_t off_flC4Blow = 0;
            static uint32_t off_flTimerLength = 0;
            static uint32_t off_nBombSite = 0;

            if (!off_bBeingDefused) off_bBeingDefused = schema::lookup("C_PlantedC4", fnv1a::runtime_hash("m_bBeingDefused"));
            if (!off_bBombDefused) off_bBombDefused = schema::lookup("C_PlantedC4", fnv1a::runtime_hash("m_bBombDefused"));
            if (!off_bHasExploded) off_bHasExploded = schema::lookup("C_PlantedC4", fnv1a::runtime_hash("m_bHasExploded"));
            if (!off_flC4Blow) off_flC4Blow = schema::lookup("C_PlantedC4", fnv1a::runtime_hash("m_flC4Blow"));
            if (!off_flTimerLength) off_flTimerLength = schema::lookup("C_PlantedC4", fnv1a::runtime_hash("m_flTimerLength"));
            if (!off_nBombSite) off_nBombSite = schema::lookup("C_PlantedC4", fnv1a::runtime_hash("m_nBombSite"));

            float current_time = memory::read<float>(global_vars_ptr + 0x30);
            float blow_time = off_flC4Blow ? memory::read<float>(planted_c4_ptr + off_flC4Blow) : 0.0f;
            bool bomb_defused = off_bBombDefused ? memory::read<bool>(planted_c4_ptr + off_bBombDefused) : false;
            bool has_exploded = off_bHasExploded ? memory::read<bool>(planted_c4_ptr + off_bHasExploded) : false;

            time_left = blow_time - current_time;

            if (!bomb_defused && !has_exploded && time_left > 0.0f) {
                is_planted = true;
                timer_length = off_flTimerLength ? memory::read<float>(planted_c4_ptr + off_flTimerLength) : 40.0f;
                if (timer_length <= 0.1f) timer_length = 40.0f;

                bomb_site = off_nBombSite ? memory::read<int>(planted_c4_ptr + off_nBombSite) : 0;
                being_defused = off_bBeingDefused ? memory::read<bool>(planted_c4_ptr + off_bBeingDefused) : false;

                uintptr_t local_pawn = valve::entity::get_local_player_pawn();
                if (local_pawn) {
                    static uint32_t off_pGameSceneNode = 0;
                    static uint32_t off_vecAbsOrigin = 0;
                    static uint32_t off_ArmorValue = 0;

                    if (!off_pGameSceneNode) off_pGameSceneNode = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pGameSceneNode"));
                    if (!off_vecAbsOrigin) off_vecAbsOrigin = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_vecAbsOrigin"));
                    if (!off_ArmorValue) off_ArmorValue = schema::lookup("C_CSPlayerPawn", fnv1a::runtime_hash("m_ArmorValue"));

                    uintptr_t c4_scene = off_pGameSceneNode ? memory::read<uintptr_t>(planted_c4_ptr + off_pGameSceneNode) : 0;
                    uintptr_t pawn_scene = off_pGameSceneNode ? memory::read<uintptr_t>(local_pawn + off_pGameSceneNode) : 0;

                    if (c4_scene && pawn_scene && off_vecAbsOrigin) {
                        vector3 c4_origin = memory::read<vector3>(c4_scene + off_vecAbsOrigin);
                        vector3 pawn_origin = memory::read<vector3>(pawn_scene + off_vecAbsOrigin);

                        vector3 diff = c4_origin - pawn_origin;
                        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

                        constexpr float default_damage = 650.0f;
                        constexpr float default_radius = 2275.0f;
                        float sigma = default_radius / 3.0f;
                        float dmg = default_damage * std::exp(-(dist * dist) / (2.0f * sigma * sigma));

                        int armor = off_ArmorValue ? memory::read<int>(local_pawn + off_ArmorValue) : 0;
                        if (armor > 0) {
                            constexpr float armor_ratio = 0.5f;
                            constexpr float armor_bonus = 0.5f;
                            float armor_absorbed = dmg * armor_ratio;
                            float armor_cost = (dmg - armor_absorbed) * armor_bonus;
                            if (armor_cost > static_cast<float>(armor)) {
                                armor_cost = static_cast<float>(armor) * (1.0f / armor_bonus);
                                armor_absorbed = dmg - armor_cost;
                            }
                            dmg = armor_absorbed;
                        }

                        damage = static_cast<int>(std::floor(dmg));
                    }
                }
            }
        }

        if (!is_planted) {
            return;
        }

        float width = 165.0f;
        float header_height = 20.0f;
        float subheader_height = 18.0f;
        float row_height = 16.0f;
        float body_height = subheader_height + 4.0f + (2.0f * row_height + 8.0f + 6.0f);
        float total_height = header_height + body_height;

        ImVec2 pos = bomb_pos;
        ImVec2 max = ImVec2(pos.x + width, pos.y + total_height);
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;

        ImRect header_bb(pos, ImVec2(max.x, pos.y + header_height));
        if (ImGui::IsMouseClicked(0) && header_bb.Contains(mouse_pos)) {
            dragging_bomb = true;
            drag_bomb_offset = ImVec2(mouse_pos.x - bomb_pos.x, mouse_pos.y - bomb_pos.y);
        }
        if (dragging_bomb) {
            if (ImGui::IsMouseDown(0)) {
                bomb_pos = ImVec2(mouse_pos.x - drag_bomb_offset.x, mouse_pos.y - drag_bomb_offset.y);
                pos = bomb_pos;
                max = ImVec2(pos.x + width, pos.y + total_height);
            } else {
                dragging_bomb = false;
            }
        }

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        draw_list->AddRect(pos, max, ImGui::GetColorU32(theme::outline_color));
        draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + header_height), ImVec2(max.x - 1, max.y - 1), ImGui::GetColorU32(theme::background_color));
        theme::draw_accent_bar(draw_list, ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, pos.y + 3), ImGui::GetColorU32(theme::accent_color));
        draw_list->AddLine(ImVec2(pos.x, pos.y + 3), ImVec2(max.x, pos.y + 3), ImGui::GetColorU32(theme::outline_color));
        theme::draw_gradient_v(
            draw_list,
            ImVec2(pos.x + 1, pos.y + 4),
            ImVec2(max.x - 1, pos.y + header_height),
            ImGui::GetColorU32(theme::inline_color),
            ImGui::GetColorU32(theme::gradient_color)
        );
        draw_list->AddLine(ImVec2(pos.x, pos.y + header_height), ImVec2(max.x, pos.y + header_height), ImGui::GetColorU32(theme::outline_color));
        theme::draw_text_stroke(draw_list, "Bomb Info", ImVec2(pos.x + 6, pos.y + 4), ImGui::GetColorU32(theme::text_color));

        float sub_y = pos.y + header_height + 4.0f;
        const char* site_str = (bomb_site == 0) ? "[Site A]" : "[Site B]";
        theme::draw_text_stroke(draw_list, being_defused ? "Defusing..." : "C4 Planted", ImVec2(pos.x + 6, sub_y), ImGui::GetColorU32(theme::text_color));
        theme::draw_text_stroke(draw_list, site_str, ImVec2(max.x - ImGui::CalcTextSize(site_str).x - 6, sub_y), ImGui::GetColorU32(theme::accent_color));

        float div_y = sub_y + 15.0f;
        draw_list->AddLine(ImVec2(pos.x + 4, div_y), ImVec2(max.x - 4, div_y), ImGui::GetColorU32(theme::gradient_color));

        float cur_y = div_y + 4.0f;

        char time_buf[32];
        snprintf(time_buf, sizeof(time_buf), "[%.1fs]", time_left);
        theme::draw_text_stroke(draw_list, "Timer", ImVec2(pos.x + 6, cur_y), ImGui::GetColorU32(theme::text_color));
        theme::draw_text_stroke(draw_list, time_buf, ImVec2(max.x - ImGui::CalcTextSize(time_buf).x - 6, cur_y), ImGui::GetColorU32(theme::accent_color));
        cur_y += row_height;

        char dmg_buf[32];
        snprintf(dmg_buf, sizeof(dmg_buf), "[-%d HP]", damage);
        theme::draw_text_stroke(draw_list, "Damage", ImVec2(pos.x + 6, cur_y), ImGui::GetColorU32(theme::text_color));
        theme::draw_text_stroke(draw_list, dmg_buf, ImVec2(max.x - ImGui::CalcTextSize(dmg_buf).x - 6, cur_y), ImGui::GetColorU32(theme::accent_color));
        cur_y += row_height + 2.0f;

        float progress = std::clamp(time_left / timer_length, 0.0f, 1.0f);
        float bar_w = (width - 12.0f) * progress;
        draw_list->AddRect(ImVec2(pos.x + 6, cur_y), ImVec2(max.x - 6, cur_y + 4), ImGui::GetColorU32(theme::outline_color));
        draw_list->AddRectFilled(ImVec2(pos.x + 7, cur_y + 1), ImVec2(max.x - 7, cur_y + 3), IM_COL32(32, 32, 32, 255));
        if (bar_w > 0.0f) {
            draw_list->AddRectFilled(ImVec2(pos.x + 7, cur_y + 1), ImVec2(pos.x + 6 + bar_w, cur_y + 3), ImGui::GetColorU32(theme::accent_color));
        }
    }

    void render() {
        int cur_font_idx = features::settings::cfg.selected_font;
        if (cur_font_idx < 0 || cur_font_idx >= (int)theme::FontID::Count)
            cur_font_idx = 0;

        ImFont* cur_font = theme::fonts[cur_font_idx] ? theme::fonts[cur_font_idx] : ImGui::GetFont();
        ImGui::PushFont(cur_font);

        features::keybinds::update();

        static bool last_key_state = false;
        bool key_down = (GetAsyncKeyState(features::settings::cfg.menu_key) & 0x8000) != 0;
        if (key_down && !last_key_state) {
            menu_open = !menu_open;
        }
        last_key_state = key_down;

        render_watermark();
        render_activity_window();
        render_spectator_window();
        render_bomb_info_window();
        notifications::render();

        if (!menu_open) {
            ImGui::PopFont();
            return;
        }

        static ImVec2 window_pos = ImVec2(20.0f, 42.0f);
        static bool dragging_main = false;
        static ImVec2 drag_main_offset = ImVec2(0, 0);

        float win_width = 475.0f;
        float win_height = 600.0f;
        ImVec2 pos = window_pos;
        ImVec2 max = ImVec2(pos.x + win_width, pos.y + win_height);
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;

        ImRect titlebar_bb(pos, ImVec2(max.x, pos.y + 20.0f));
        if (ImGui::IsMouseClicked(0) && titlebar_bb.Contains(mouse_pos)) {
            dragging_main = true;
            drag_main_offset = ImVec2(mouse_pos.x - window_pos.x, mouse_pos.y - window_pos.y);
        }
        if (dragging_main) {
            if (ImGui::IsMouseDown(0)) {
                window_pos = ImVec2(mouse_pos.x - drag_main_offset.x, mouse_pos.y - drag_main_offset.y);
                pos = window_pos;
                max = ImVec2(pos.x + win_width, pos.y + win_height);
            } else {
                dragging_main = false;
            }
        }

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(ImVec2(win_width, win_height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("##aurahook_main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            draw_list->AddRectFilled(pos, max, ImGui::GetColorU32(theme::outline_color));
            draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + 4), ImVec2(max.x - 1, max.y - 1), ImGui::GetColorU32(theme::gradient_color));
            draw_list->AddRect(pos, max, ImGui::GetColorU32(theme::outline_color));

            theme::draw_accent_bar(draw_list, ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, pos.y + 3), ImGui::GetColorU32(theme::accent_color));
            draw_list->AddLine(ImVec2(pos.x, pos.y + 3), ImVec2(max.x, pos.y + 3), ImGui::GetColorU32(theme::outline_color));

            theme::draw_gradient_v(
                draw_list,
                ImVec2(pos.x + 2, pos.y + 5),
                ImVec2(max.x - 2, pos.y + 21),
                ImGui::GetColorU32(theme::inline_color),
                ImGui::GetColorU32(theme::gradient_color)
            );

            const char* menu_title = (features::settings::cfg.custom_menu_name && features::settings::cfg.menu_name[0] != '\0') ? features::settings::cfg.menu_name : "AuraHook";
            ImVec2 title_sz = ImGui::CalcTextSize(menu_title);
            float title_y = pos.y + 5.0f + (16.0f - title_sz.y) * 0.5f;
            theme::draw_text_stroke(draw_list, menu_title, ImVec2(pos.x + 6, title_y), ImGui::GetColorU32(theme::text_color));

            ImVec2 in_min = ImVec2(pos.x + 5, pos.y + 22);
            ImVec2 in_max = ImVec2(max.x - 5, max.y - 5);
            draw_list->AddRect(in_min, in_max, ImGui::GetColorU32(theme::inline_color));

            ImVec2 out_min = ImVec2(in_min.x + 1, in_min.y + 1);
            ImVec2 out_max = ImVec2(in_max.x - 1, in_max.y - 1);
            draw_list->AddRect(out_min, out_max, ImGui::GetColorU32(theme::outline_color));

            ImVec2 bg_min = ImVec2(out_min.x + 1, out_min.y + 1);
            ImVec2 bg_max = ImVec2(out_max.x - 1, out_max.y - 1);
            draw_list->AddRectFilled(bg_min, bg_max, ImGui::GetColorU32(theme::background_color));

            const float tab_bar_height = 36.0f;
            ImVec2 tab_bar_min = ImVec2(bg_min.x, bg_min.y);
            ImVec2 tab_bar_max = ImVec2(bg_max.x, bg_min.y + tab_bar_height);

            theme::draw_gradient_v(
                draw_list,
                tab_bar_min,
                tab_bar_max,
                ImGui::GetColorU32(theme::gradient_color),
                ImGui::GetColorU32(theme::background_color)
            );

            theme::draw_accent_bar(draw_list, tab_bar_min, ImVec2(tab_bar_max.x, tab_bar_min.y + 2.0f), ImGui::GetColorU32(theme::accent_color));

            draw_list->AddLine(
                ImVec2(out_min.x, tab_bar_max.y),
                ImVec2(out_max.x, tab_bar_max.y),
                ImGui::GetColorU32(theme::outline_color)
            );

            const char* tab_names[] = { "Legit", "Rage", "Visuals", "Misc", "Settings" };
            int tab_count = 5;

            float tab_bar_w = tab_bar_max.x - tab_bar_min.x;
            float tab_w = tab_bar_w / (float)tab_count;

            ImFont* tab_font = theme::fonts_tabs[cur_font_idx] ? theme::fonts_tabs[cur_font_idx] : cur_font;

            for (int i = 0; i < tab_count; i++) {
                float btn_x = bg_min.x + (float)i * tab_w;
                float btn_next_x = (i == tab_count - 1) ? bg_max.x : (bg_min.x + (float)(i + 1) * tab_w);

                ImVec2 btn_min = ImVec2(btn_x, tab_bar_min.y + 2.0f);
                ImVec2 btn_max = ImVec2(btn_next_x, tab_bar_max.y);
                ImRect tab_bb(btn_min, btn_max);

                bool is_active = (current_tab == static_cast<Tab>(i));
                bool is_hovered = tab_bb.Contains(mouse_pos);

                if (is_hovered && ImGui::IsMouseClicked(0)) {
                    current_tab = static_cast<Tab>(i);
                    is_active = true;
                }

                ImGui::PushFont(tab_font);
                ImVec2 text_sz = ImGui::CalcTextSize(tab_names[i]);
                ImVec2 text_pos = ImVec2(btn_min.x + (btn_max.x - btn_min.x - text_sz.x) * 0.5f, btn_min.y + (btn_max.y - btn_min.y - text_sz.y) * 0.5f);

                if (is_active) {
                    draw_list->AddRectFilled(btn_min, btn_max, ImGui::GetColorU32(theme::background_color));
                    draw_list->AddLine(btn_min, ImVec2(btn_max.x, btn_min.y), ImGui::GetColorU32(theme::outline_color));
                    if (i > 0)
                        draw_list->AddLine(btn_min, ImVec2(btn_min.x, btn_max.y), ImGui::GetColorU32(theme::outline_color));
                    if (i < tab_count - 1)
                        draw_list->AddLine(ImVec2(btn_max.x, btn_min.y), btn_max, ImGui::GetColorU32(theme::outline_color));
                    draw_list->AddLine(
                        ImVec2(btn_min.x, btn_max.y),
                        ImVec2(btn_max.x, btn_max.y),
                        ImGui::GetColorU32(theme::background_color)
                    );
                    theme::draw_text_stroke(draw_list, tab_names[i], text_pos, ImGui::GetColorU32(theme::text_color));
                } else {
                    draw_list->AddRectFilled(btn_min, btn_max, ImGui::GetColorU32(theme::tab_bg_color));
                    draw_list->AddLine(btn_min, ImVec2(btn_max.x, btn_min.y), ImGui::GetColorU32(theme::outline_color));
                    draw_list->AddLine(ImVec2(btn_min.x, btn_max.y), ImVec2(btn_max.x, btn_max.y), ImGui::GetColorU32(theme::outline_color));
                    if (i > 0)
                        draw_list->AddLine(btn_min, ImVec2(btn_min.x, btn_max.y), ImGui::GetColorU32(theme::outline_color));
                    if (i < tab_count - 1)
                        draw_list->AddLine(ImVec2(btn_max.x, btn_min.y), btn_max, ImGui::GetColorU32(theme::outline_color));
                    ImU32 text_col = is_hovered ? ImGui::GetColorU32(theme::text_color) : ImGui::GetColorU32(theme::text_dim_color);
                    theme::draw_text_stroke(draw_list, tab_names[i], text_pos, text_col);
                }
                ImGui::PopFont();
            }

            ImVec2 page_min = ImVec2(bg_min.x + 4.0f, tab_bar_max.y + 4.0f);
            ImVec2 page_max = ImVec2(bg_max.x - 4.0f, bg_max.y - 4.0f);

            ImGui::SetCursorScreenPos(page_min);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            if (ImGui::BeginChild("##aurahook_page", ImVec2(page_max.x - page_min.x, page_max.y - page_min.y), false, ImGuiWindowFlags_NoBackground)) {
                switch (current_tab) {
                    case Tab::Legit:
                        features::legit::render();
                        break;
                    case Tab::Rage:
                        features::rage::render();
                        break;
                    case Tab::Visuals:
                        features::visuals::render();
                        break;
                    case Tab::Misc:
                        features::misc::render();
                        break;
                    case Tab::Settings:
                        features::settings::render();
                        break;
                    default:
                        break;
                }
            }
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }

}

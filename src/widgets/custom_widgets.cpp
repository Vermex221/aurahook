#include "custom_widgets.hpp"
#include "../imgui/imgui_internal.h"
#include "../gui/theme/theme.hpp"
#include "../gui/gui/gui.hpp"
#include "../features/settings/settings.hpp"
#include "../valve/icons/icons.hpp"

#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

namespace widgets {

    static int active_keybind_id = -1;
    static bool keybind_waiting_release = false;

    struct SectionState {
        std::string title;
        ImVec2 pos;
        float width;
        float height;
        float header_height;
        bool is_subtab_section = false;
        int* selected_subtab = nullptr;
        std::vector<std::string> subtab_names;
    };

    static std::vector<SectionState> section_stack;
    static std::unordered_map<std::string, float> section_heights;

    static float get_widget_width() {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window && (window->Flags & ImGuiWindowFlags_Popup)) {
            return ImGui::GetContentRegionAvail().x;
        }
        if (!section_stack.empty()) {
            return section_stack.back().width - 12.0f;
        }
        return ImGui::GetContentRegionAvail().x;
    }

    const char* get_key_name(int key) {
        if (key == 0) return "-";
        if (key == VK_LBUTTON) return "MB1";
        if (key == VK_RBUTTON) return "MB2";
        if (key == VK_MBUTTON) return "MB3";
        if (key == VK_XBUTTON1) return "MB4";
        if (key == VK_XBUTTON2) return "MB5";
        if (key == VK_SHIFT || key == VK_LSHIFT) return "Shift";
        if (key == VK_RSHIFT) return "RShift";
        if (key == VK_CONTROL || key == VK_LCONTROL) return "Ctrl";
        if (key == VK_RCONTROL) return "RCtrl";
        if (key == VK_MENU || key == VK_LMENU) return "Alt";
        if (key == VK_RMENU) return "RAlt";
        if (key == VK_SPACE) return "Space";
        if (key == VK_TAB) return "Tab";
        if (key == VK_CAPITAL) return "Caps";
        if (key == VK_ESCAPE) return "Esc";
        if (key == VK_RETURN) return "Enter";
        if (key == VK_BACK) return "Back";
        if (key == VK_INSERT) return "Insert";
        if (key == VK_DELETE) return "Delete";
        if (key == VK_HOME) return "Home";
        if (key == VK_END) return "End";
        if (key == VK_PRIOR) return "PageUp";
        if (key == VK_NEXT) return "PageDown";
        if (key == VK_UP) return "Up";
        if (key == VK_DOWN) return "Down";
        if (key == VK_LEFT) return "Left";
        if (key == VK_RIGHT) return "Right";
        if (key == VK_LWIN || key == VK_RWIN) return "Win";
        if (key == VK_NUMLOCK) return "NumLock";
        if (key == VK_SCROLL) return "Scroll";
        if (key == VK_PAUSE) return "Pause";
        if (key == VK_SNAPSHOT) return "Print";

        if (key >= 'A' && key <= 'Z') {
            static char buf[2] = { 0, 0 };
            buf[0] = (char)key;
            return buf;
        }
        if (key >= '0' && key <= '9') {
            static char buf[2] = { 0, 0 };
            buf[0] = (char)key;
            return buf;
        }
        if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
            static char buf[8];
            snprintf(buf, sizeof(buf), "Num %d", key - VK_NUMPAD0);
            return buf;
        }
        if (key == VK_MULTIPLY) return "Num *";
        if (key == VK_ADD) return "Num +";
        if (key == VK_SUBTRACT) return "Num -";
        if (key == VK_DECIMAL) return "Num .";
        if (key == VK_DIVIDE) return "Num /";

        if (key == VK_OEM_3) return "~";
        if (key == VK_OEM_MINUS) return "-";
        if (key == VK_OEM_PLUS) return "=";
        if (key == VK_OEM_4) return "[";
        if (key == VK_OEM_6) return "]";
        if (key == VK_OEM_5) return "\\";
        if (key == VK_OEM_1) return ";";
        if (key == VK_OEM_7) return "'";
        if (key == VK_OEM_COMMA) return ",";
        if (key == VK_OEM_PERIOD) return ".";
        if (key == VK_OEM_2) return "/";

        if (key >= VK_F1 && key <= VK_F12) {
            static char buf[8];
            snprintf(buf, sizeof(buf), "F%d", key - VK_F1 + 1);
            return buf;
        }

        return "-";
    }

    bool begin_section(const char* title, float width, float height) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float w = (width > 0.0f) ? width : avail.x;
        bool has_title = (title && title[0] != '\0');
        float header_height = has_title ? 16.0f : 4.0f;

        float h = height;
        if (h <= 0.0f) {
            std::string key = (title && title[0]) ? title : "##notitle";
            auto it = section_heights.find(key);
            h = (it != section_heights.end() && it->second > 0.0f) ? it->second : 220.0f;
        }

        ImVec2 pos = window->DC.CursorPos;
        ImDrawList* draw_list = window->DrawList;

        
        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(1);

        
        SectionState s;
        s.title = title ? title : "";
        s.pos = pos;
        s.width = w;
        s.height = height;
        s.header_height = header_height;
        s.is_subtab_section = false;
        section_stack.push_back(s);

        
        ImGui::PushID(title ? title : "##sec");

        
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 6.0f, pos.y + header_height + 2.0f));
        ImGui::PushItemWidth(w - 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 3.0f));
        ImGui::BeginGroup();

        return true;
    }

    bool begin_section_subtabs(int* selected_subtab, const char* const subtab_names[], int subtab_count, float width, float height) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float w = (width > 0.0f) ? width : avail.x;
        float header_height = 18.0f;

        float h = height;
        if (h <= 0.0f) {
            std::string key = (subtab_names && subtab_count > 0) ? subtab_names[0] : "##subtab_sec";
            auto it = section_heights.find(key);
            h = (it != section_heights.end() && it->second > 0.0f) ? it->second : 220.0f;
        }

        ImVec2 pos = window->DC.CursorPos;
        ImDrawList* draw_list = window->DrawList;

        draw_list->ChannelsSplit(2);
        draw_list->ChannelsSetCurrent(1);

        SectionState s;
        s.title = (subtab_names && subtab_count > 0) ? subtab_names[0] : "##subtab_sec";
        s.pos = pos;
        s.width = w;
        s.height = height;
        s.header_height = header_height;
        s.is_subtab_section = true;
        s.selected_subtab = selected_subtab;
        for (int i = 0; i < subtab_count; i++) {
            s.subtab_names.push_back(subtab_names[i]);
        }
        section_stack.push_back(s);

        ImGui::PushID(s.title.c_str());

        ImGui::SetCursorScreenPos(ImVec2(pos.x + 6.0f, pos.y + header_height + 4.0f));
        ImGui::PushItemWidth(w - 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 3.0f));
        ImGui::BeginGroup();

        return true;
    }

    void end_section() {
        ImGui::EndGroup();
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();
        ImGui::PopID();

        if (!section_stack.empty()) {
            SectionState s = section_stack.back();
            section_stack.pop_back();

            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImDrawList* draw_list = window->DrawList;
            ImVec2 mouse = ImGui::GetIO().MousePos;

            float items_h = ImGui::GetItemRectSize().y;
            float actual_h = (s.height > 0.0f) ? s.height : (s.header_height + items_h + 6.0f);
            std::string key = s.title.empty() ? "##notitle" : s.title;
            section_heights[key] = actual_h;

            
            draw_list->ChannelsSetCurrent(0);

            ImVec2 pos = s.pos;
            ImVec2 max = ImVec2(pos.x + s.width, pos.y + actual_h);

            
            draw_list->AddRect(pos, max, ImGui::GetColorU32(gui::theme::outline_color));

            
            draw_list->AddRectFilled(ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, max.y - 1), ImGui::GetColorU32(gui::theme::background_color));

            
            gui::theme::draw_accent_bar(draw_list, ImVec2(pos.x + 1, pos.y + 1), ImVec2(max.x - 1, pos.y + 3), ImGui::GetColorU32(gui::theme::accent_color));

            if (s.is_subtab_section) {
                float cur_x = pos.x;
                float tab_top_y = pos.y + 3.0f;
                float tab_bot_y = pos.y + s.header_height + 1.0f;

                for (size_t i = 0; i < s.subtab_names.size(); i++) {
                    ImVec2 text_sz = ImGui::CalcTextSize(s.subtab_names[i].c_str());
                    float tab_w = text_sz.x + 14.0f;
                    ImVec2 tab_min = ImVec2(cur_x, tab_top_y);
                    ImVec2 tab_max = ImVec2(cur_x + tab_w, tab_bot_y);

                    ImRect tab_bb(tab_min, tab_max);
                    bool hovered = tab_bb.Contains(mouse);
                    bool is_active = (s.selected_subtab && *s.selected_subtab == (int)i);

                    if (hovered && ImGui::IsMouseClicked(0)) {
                        if (s.selected_subtab) *s.selected_subtab = (int)i;
                        is_active = true;
                    }

                    if (is_active) {
                        draw_list->AddRectFilled(
                            ImVec2(tab_min.x + 1.0f, tab_min.y + 1.0f),
                            ImVec2(tab_max.x - 1.0f, tab_max.y),
                            ImGui::GetColorU32(gui::theme::background_color)
                        );

                        draw_list->AddLine(tab_min, ImVec2(tab_max.x, tab_min.y), ImGui::GetColorU32(gui::theme::outline_color));
                        draw_list->AddLine(tab_min, ImVec2(tab_min.x, tab_max.y), ImGui::GetColorU32(gui::theme::outline_color));
                        draw_list->AddLine(ImVec2(tab_max.x, tab_min.y), tab_max, ImGui::GetColorU32(gui::theme::outline_color));

                        draw_list->AddLine(
                            ImVec2(tab_min.x + 1.0f, tab_max.y),
                            ImVec2(tab_max.x - 1.0f, tab_max.y),
                            ImGui::GetColorU32(gui::theme::background_color)
                        );

                        ImVec2 text_pos = ImVec2(tab_min.x + (tab_w - text_sz.x) * 0.5f, tab_min.y + (tab_bot_y - tab_top_y - text_sz.y) * 0.5f - 0.5f);
                        gui::theme::draw_text_stroke(draw_list, s.subtab_names[i].c_str(), text_pos, ImGui::GetColorU32(gui::theme::text_color));
                    } else {
                        draw_list->AddRectFilled(ImVec2(tab_min.x + 1.0f, tab_min.y + 1.0f), ImVec2(tab_max.x - 1.0f, tab_max.y), ImGui::GetColorU32(gui::theme::tab_bg_color));
                        draw_list->AddRect(tab_min, tab_max, ImGui::GetColorU32(gui::theme::outline_color));

                        ImU32 text_col = hovered ? ImGui::GetColorU32(gui::theme::text_color) : ImGui::GetColorU32(gui::theme::text_dim_color);
                        ImVec2 text_pos = ImVec2(tab_min.x + (tab_w - text_sz.x) * 0.5f, tab_min.y + (tab_bot_y - tab_top_y - text_sz.y) * 0.5f - 0.5f);
                        gui::theme::draw_text_stroke(draw_list, s.subtab_names[i].c_str(), text_pos, text_col);
                    }

                    cur_x += tab_w;
                }

                draw_list->AddLine(ImVec2(cur_x, tab_bot_y), ImVec2(max.x, tab_bot_y), ImGui::GetColorU32(gui::theme::outline_color));
            } else if (!s.title.empty()) {
                draw_list->AddLine(ImVec2(pos.x, pos.y + 3), ImVec2(max.x, pos.y + 3), ImGui::GetColorU32(gui::theme::outline_color));
                gui::theme::draw_text_stroke(draw_list, s.title.c_str(), ImVec2(pos.x + 6, pos.y + 4), ImGui::GetColorU32(gui::theme::text_color));
            }

            draw_list->ChannelsMerge();

            
            ImGui::SetCursorScreenPos(s.pos);
            ImGui::Dummy(ImVec2(s.width, actual_h));
        }
    }

    bool subtabs(int* selected_tab, const char* const tab_names[], int count) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID("##subtabs");
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float tab_h = 20.0f;

        ImDrawList* draw_list = window->DrawList;
        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool changed = false;

        float tab_w = width / (float)count;

        draw_list->AddLine(
            ImVec2(pos.x, pos.y + tab_h),
            ImVec2(pos.x + width, pos.y + tab_h),
            ImGui::GetColorU32(gui::theme::outline_color)
        );

        for (int i = 0; i < count; i++) {
            float btn_x = pos.x + (float)i * tab_w;
            float btn_next_x = (i == count - 1) ? (pos.x + width) : (pos.x + (float)(i + 1) * tab_w);

            ImVec2 btn_min = ImVec2(btn_x, pos.y);
            ImVec2 btn_max = ImVec2(btn_next_x, pos.y + tab_h);

            ImRect tab_bb(btn_min, btn_max);
            bool hovered = tab_bb.Contains(mouse);
            bool is_active = (*selected_tab == i);

            if (hovered && ImGui::IsMouseClicked(0)) {
                *selected_tab = i;
                changed = true;
                is_active = true;
            }

            ImVec2 text_sz = ImGui::CalcTextSize(tab_names[i]);
            ImVec2 text_pos = ImVec2(btn_min.x + (btn_max.x - btn_min.x - text_sz.x) * 0.5f, btn_min.y + (tab_h - text_sz.y) * 0.5f - 0.5f);

            if (is_active) {
                draw_list->AddRectFilled(
                    ImVec2(btn_min.x + 1, btn_min.y + 1),
                    ImVec2(btn_max.x - 1, btn_max.y),
                    ImGui::GetColorU32(gui::theme::background_color)
                );

                draw_list->AddLine(btn_min, ImVec2(btn_max.x, btn_min.y), ImGui::GetColorU32(gui::theme::outline_color));
                draw_list->AddLine(btn_min, ImVec2(btn_min.x, btn_max.y), ImGui::GetColorU32(gui::theme::outline_color));
                draw_list->AddLine(ImVec2(btn_max.x, btn_min.y), btn_max, ImGui::GetColorU32(gui::theme::outline_color));

                draw_list->AddLine(
                    ImVec2(btn_min.x + 1, btn_max.y),
                    ImVec2(btn_max.x - 1, btn_max.y),
                    ImGui::GetColorU32(gui::theme::background_color)
                );

                gui::theme::draw_text_stroke(draw_list, tab_names[i], text_pos, ImGui::GetColorU32(gui::theme::text_color));
            } else {
                draw_list->AddRectFilled(btn_min, btn_max, ImGui::GetColorU32(gui::theme::tab_bg_color));
                draw_list->AddRect(btn_min, btn_max, ImGui::GetColorU32(gui::theme::outline_color));

                ImU32 text_col = hovered ? ImGui::GetColorU32(gui::theme::text_color) : ImGui::GetColorU32(gui::theme::text_dim_color);
                gui::theme::draw_text_stroke(draw_list, tab_names[i], text_pos, text_col);
            }
        }

        ImRect total_bb(pos, ImVec2(pos.x + width, pos.y + tab_h));
        ImGui::ItemSize(total_bb);
        if (!ImGui::ItemAdd(total_bb, id))
            return false;

        ImGui::Spacing();

        return changed;
    }

    bool subtab_bar(int* selected_subtab, const char* const subtab_names[], int subtab_count) {
        if (!selected_subtab || !subtab_names || subtab_count <= 0) return false;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImDrawList* draw_list = window->DrawList;
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;

        float bar_width = ImGui::GetContentRegionAvail().x;
        float bar_height = 32.0f;

        ImVec2 min_pos = ImGui::GetCursorScreenPos();
        ImVec2 max_pos = ImVec2(min_pos.x + bar_width, min_pos.y + bar_height);

        ImGui::SetCursorScreenPos(ImVec2(min_pos.x, min_pos.y + bar_height + 6.0f));

        gui::theme::draw_gradient_v(
            draw_list,
            min_pos,
            max_pos,
            ImGui::GetColorU32(gui::theme::gradient_color),
            ImGui::GetColorU32(gui::theme::background_color)
        );

        gui::theme::draw_accent_bar(draw_list, min_pos, ImVec2(max_pos.x, min_pos.y + 2.0f), ImGui::GetColorU32(gui::theme::accent_color));

        draw_list->AddLine(
            ImVec2(min_pos.x, max_pos.y),
            ImVec2(max_pos.x, max_pos.y),
            ImGui::GetColorU32(gui::theme::outline_color)
        );

        float tab_w = bar_width / static_cast<float>(subtab_count);
        bool changed = false;

        int cur_font_idx = features::settings::cfg.selected_font;
        if (cur_font_idx < 0 || cur_font_idx >= (int)gui::theme::FontID::Count)
            cur_font_idx = 0;
        ImFont* tab_font = gui::theme::fonts_tabs[cur_font_idx] ? gui::theme::fonts_tabs[cur_font_idx] : ImGui::GetFont();

        for (int i = 0; i < subtab_count; i++) {
            float btn_x = min_pos.x + static_cast<float>(i) * tab_w;
            float btn_next_x = (i == subtab_count - 1) ? max_pos.x : (min_pos.x + static_cast<float>(i + 1) * tab_w);

            ImVec2 btn_min = ImVec2(btn_x, min_pos.y + 2.0f);
            ImVec2 btn_max = ImVec2(btn_next_x, max_pos.y);
            ImRect tab_bb(btn_min, btn_max);

            bool is_active = (*selected_subtab == i);
            bool is_hovered = tab_bb.Contains(mouse_pos);

            if (is_hovered && ImGui::IsMouseClicked(0)) {
                if (*selected_subtab != i) {
                    *selected_subtab = i;
                    changed = true;
                }
                is_active = true;
            }

            ImGui::PushFont(tab_font);
            ImVec2 text_sz = ImGui::CalcTextSize(subtab_names[i]);
            ImVec2 text_pos = ImVec2(btn_min.x + (btn_max.x - btn_min.x - text_sz.x) * 0.5f, btn_min.y + (btn_max.y - btn_min.y - text_sz.y) * 0.5f);

            auto* icon_data = valve::icons::get(subtab_names[i], 0.56f);

            if (is_active) {
                draw_list->AddRectFilled(btn_min, btn_max, ImGui::GetColorU32(gui::theme::background_color));
                draw_list->AddLine(btn_min, ImVec2(btn_max.x, btn_min.y), ImGui::GetColorU32(gui::theme::outline_color));
                if (i > 0)
                    draw_list->AddLine(btn_min, ImVec2(btn_min.x, btn_max.y), ImGui::GetColorU32(gui::theme::outline_color));
                if (i < subtab_count - 1)
                    draw_list->AddLine(ImVec2(btn_max.x, btn_min.y), btn_max, ImGui::GetColorU32(gui::theme::outline_color));
                draw_list->AddLine(
                    ImVec2(btn_min.x, btn_max.y),
                    ImVec2(btn_max.x, btn_max.y),
                    ImGui::GetColorU32(gui::theme::background_color)
                );

                if (icon_data && icon_data->texture) {
                    float iw = icon_data->width;
                    float ih = icon_data->height;
                    float ix = std::floor(btn_min.x + (btn_max.x - btn_min.x - iw) * 0.5f);
                    float iy = std::floor(btn_min.y + (btn_max.y - btn_min.y - ih) * 0.5f);
                    draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(gui::theme::text_color));
                } else {
                    gui::theme::draw_text_stroke(draw_list, subtab_names[i], text_pos, ImGui::GetColorU32(gui::theme::text_color));
                }
            } else {
                draw_list->AddRectFilled(btn_min, btn_max, ImGui::GetColorU32(gui::theme::tab_bg_color));
                draw_list->AddLine(btn_min, ImVec2(btn_max.x, btn_min.y), ImGui::GetColorU32(gui::theme::outline_color));
                draw_list->AddLine(ImVec2(btn_min.x, btn_max.y), ImVec2(btn_max.x, btn_max.y), ImGui::GetColorU32(gui::theme::outline_color));
                if (i > 0)
                    draw_list->AddLine(btn_min, ImVec2(btn_min.x, btn_max.y), ImGui::GetColorU32(gui::theme::outline_color));
                if (i < subtab_count - 1)
                    draw_list->AddLine(ImVec2(btn_max.x, btn_min.y), btn_max, ImGui::GetColorU32(gui::theme::outline_color));

                ImU32 col = is_hovered ? ImGui::GetColorU32(gui::theme::text_color) : ImGui::GetColorU32(gui::theme::text_dim_color);
                if (icon_data && icon_data->texture) {
                    float iw = icon_data->width;
                    float ih = icon_data->height;
                    float ix = std::floor(btn_min.x + (btn_max.x - btn_min.x - iw) * 0.5f);
                    float iy = std::floor(btn_min.y + (btn_max.y - btn_min.y - ih) * 0.5f);
                    draw_list->AddImage((ImTextureID)icon_data->texture, ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), ImVec2(0, 0), ImVec2(1, 1), col);
                } else {
                    gui::theme::draw_text_stroke(draw_list, subtab_names[i], text_pos, col);
                }
            }
            ImGui::PopFont();
        }

        return changed;
    }

    bool toggle(const char* label, bool* value) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float height = 15.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered = ImGui::IsItemHovered();
        bool clicked = hovered && ImGui::IsMouseClicked(0);

        if (clicked) {
            *value = !*value;
        }

        ImDrawList* draw_list = window->DrawList;

        
        ImVec2 box_min = ImVec2(pos.x, pos.y + 2.5f);
        ImVec2 box_max = ImVec2(pos.x + 10.0f, pos.y + 12.5f);

        
        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));

        
        ImVec2 inner_min = ImVec2(box_min.x + 1, box_min.y + 1);
        ImVec2 inner_max = ImVec2(box_max.x - 1, box_max.y - 1);

        if (*value) {
            
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::accent_color));
        } else {
            
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::background_color));
            draw_list->AddRect(inner_min, inner_max, ImGui::GetColorU32(gui::theme::inline_color));
        }

        
        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x + 15.0f, pos.y + 1.5f), ImGui::GetColorU32(gui::theme::text_color));

        return clicked;
    }

    bool toggle_with_color(const char* label, bool* value, float color[4], const char* color_title) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float height = 15.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        float right_x = pos.x + width;
        if (!section_stack.empty() && !(window->Flags & ImGuiWindowFlags_Popup)) {
            const auto& s = section_stack.back();
            right_x = s.pos.x + s.width - 6.0f;
        }

        float badge_w = 20.0f;
        ImVec2 badge_pos = ImVec2(right_x - badge_w, pos.y + 3.0f);
        ImRect badge_bb(badge_pos, ImVec2(badge_pos.x + badge_w, badge_pos.y + 9.0f));

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool badge_hovered = badge_bb.Contains(mouse);
        bool toggle_hovered = bb.Contains(mouse) && !badge_hovered;

        bool clicked = toggle_hovered && ImGui::IsMouseClicked(0);
        if (clicked) {
            *value = !*value;
        }

        ImDrawList* draw_list = window->DrawList;
        ImVec2 box_min = ImVec2(pos.x, pos.y + 2.5f);
        ImVec2 box_max = ImVec2(pos.x + 10.0f, pos.y + 12.5f);

        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));
        ImVec2 inner_min = ImVec2(box_min.x + 1, box_min.y + 1);
        ImVec2 inner_max = ImVec2(box_max.x - 1, box_max.y - 1);

        if (*value) {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::accent_color));
        } else {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::background_color));
            draw_list->AddRect(inner_min, inner_max, ImGui::GetColorU32(gui::theme::inline_color));
        }

        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x + 15.0f, pos.y + 1.5f), ImGui::GetColorU32(gui::theme::text_color));

        ImGui::PushID(label);
        ImVec2 cur_backup = window->DC.CursorPos;
        ImGui::SetCursorScreenPos(badge_pos);
        const char* actual_title = color_title ? color_title : (strcmp(label, "Menu Accent") == 0 ? "Accent Color" : label);
        bool cp_changed = colorpicker("##cp", color, actual_title);
        if (cp_changed) {
            if (strcmp(actual_title, "Accent Color") == 0 || strcmp(label, "Menu Accent") == 0) {
                gui::theme::accent_color = ImVec4(color[0], color[1], color[2], color[3]);
                gui::theme::apply();
            }
        }
        window->DC.CursorPos = cur_backup;
        window->DC.IsSetPos = false;
        ImGui::PopID();

        return clicked || cp_changed;
    }

    bool toggle_with_two_colors(const char* label, bool* value, float color1[4], float color2[4], const char* title1, const char* title2) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float height = 15.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        float right_x = pos.x + width;
        if (!section_stack.empty() && !(window->Flags & ImGuiWindowFlags_Popup)) {
            const auto& s = section_stack.back();
            right_x = s.pos.x + s.width - 6.0f;
        }

        float badge_w = 20.0f;
        ImVec2 badge2_pos = ImVec2(right_x - badge_w, pos.y + 3.0f);
        ImVec2 badge1_pos = ImVec2(right_x - badge_w * 2.0f - 4.0f, pos.y + 3.0f);
        ImRect badge2_bb(badge2_pos, ImVec2(badge2_pos.x + badge_w, badge2_pos.y + 9.0f));
        ImRect badge1_bb(badge1_pos, ImVec2(badge1_pos.x + badge_w, badge1_pos.y + 9.0f));

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool b1_hov = badge1_bb.Contains(mouse);
        bool b2_hov = badge2_bb.Contains(mouse);
        bool toggle_hovered = bb.Contains(mouse) && !b1_hov && !b2_hov;

        bool clicked = toggle_hovered && ImGui::IsMouseClicked(0);
        if (clicked) {
            *value = !*value;
        }

        ImDrawList* draw_list = window->DrawList;
        ImVec2 box_min = ImVec2(pos.x, pos.y + 2.5f);
        ImVec2 box_max = ImVec2(pos.x + 10.0f, pos.y + 12.5f);

        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));
        ImVec2 inner_min = ImVec2(box_min.x + 1, box_min.y + 1);
        ImVec2 inner_max = ImVec2(box_max.x - 1, box_max.y - 1);

        if (*value) {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::accent_color));
        } else {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::background_color));
            draw_list->AddRect(inner_min, inner_max, ImGui::GetColorU32(gui::theme::inline_color));
        }

        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x + 15.0f, pos.y + 1.5f), ImGui::GetColorU32(gui::theme::text_color));

        ImGui::PushID(label);
        ImVec2 cur_backup = window->DC.CursorPos;

        ImGui::SetCursorScreenPos(badge1_pos);
        bool cp1_changed = colorpicker("##cp1", color1, title1 ? title1 : "Visible Color");

        ImGui::SetCursorScreenPos(badge2_pos);
        bool cp2_changed = colorpicker("##cp2", color2, title2 ? title2 : "Invisible Color");

        window->DC.CursorPos = cur_backup;
        window->DC.IsSetPos = false;
        ImGui::PopID();

        return clicked || cp1_changed || cp2_changed;
    }

    bool toggle_with_keybind(const char* label, bool* value, int* key, int* mode) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float height = 15.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        float right_x = pos.x + width;
        if (!section_stack.empty() && !(window->Flags & ImGuiWindowFlags_Popup)) {
            const auto& s = section_stack.back();
            right_x = s.pos.x + s.width - 6.0f;
        }

        const char* key_str = get_key_name(*key);
        ImVec2 text_size = ImGui::CalcTextSize(key_str);
        float kb_w = (std::max)(28.0f, text_size.x + 10.0f);
        ImVec2 kb_pos = ImVec2(right_x - kb_w, pos.y + 0.5f);
        ImRect kb_bb(kb_pos, ImVec2(kb_pos.x + kb_w, kb_pos.y + 15.0f));

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool kb_hovered = kb_bb.Contains(mouse);
        bool toggle_hovered = bb.Contains(mouse) && !kb_hovered;

        bool clicked = toggle_hovered && ImGui::IsMouseClicked(0);
        if (clicked) {
            *value = !*value;
        }

        ImDrawList* draw_list = window->DrawList;
        ImVec2 box_min = ImVec2(pos.x, pos.y + 2.5f);
        ImVec2 box_max = ImVec2(pos.x + 10.0f, pos.y + 12.5f);

        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));
        ImVec2 inner_min = ImVec2(box_min.x + 1, box_min.y + 1);
        ImVec2 inner_max = ImVec2(box_max.x - 1, box_max.y - 1);

        if (*value) {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::accent_color));
        } else {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::background_color));
            draw_list->AddRect(inner_min, inner_max, ImGui::GetColorU32(gui::theme::inline_color));
        }

        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x + 15.0f, pos.y + 1.5f), ImGui::GetColorU32(gui::theme::text_color));

        ImGui::PushID(label);
        ImVec2 cur_backup = window->DC.CursorPos;
        ImGui::SetCursorScreenPos(kb_pos);
        keybind("##kb", key, mode);
        window->DC.CursorPos = cur_backup;
        window->DC.IsSetPos = false;
        ImGui::PopID();

        return clicked;
    }

    bool toggle_with_settings(const char* label, bool* value, const char* title, const std::function<void()>& render_fn, float popup_width) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float height = 15.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        float right_x = pos.x + width;
        if (!section_stack.empty() && !(window->Flags & ImGuiWindowFlags_Popup)) {
            const auto& s = section_stack.back();
            right_x = s.pos.x + s.width - 6.0f;
        }

        const char* btn_text = "...";
        ImVec2 text_size = ImGui::CalcTextSize(btn_text);
        float btn_w = 20.0f;
        float btn_h = 15.0f;
        ImVec2 btn_pos = ImVec2(right_x - btn_w, pos.y + 0.5f);
        ImRect btn_bb(btn_pos, ImVec2(btn_pos.x + btn_w, btn_pos.y + btn_h));

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool btn_hovered = btn_bb.Contains(mouse);
        bool toggle_hovered = bb.Contains(mouse) && !btn_hovered;

        bool clicked = toggle_hovered && ImGui::IsMouseClicked(0);
        if (clicked) {
            *value = !*value;
        }

        char popup_id[64];
        snprintf(popup_id, sizeof(popup_id), "##stgpop_%s", label);

        if ((btn_hovered && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) || (toggle_hovered && ImGui::IsMouseClicked(1))) {
            ImGui::OpenPopup(popup_id);
        }

        ImDrawList* draw_list = window->DrawList;
        ImVec2 box_min = ImVec2(pos.x, pos.y + 2.5f);
        ImVec2 box_max = ImVec2(pos.x + 10.0f, pos.y + 12.5f);

        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));
        ImVec2 inner_min = ImVec2(box_min.x + 1, box_min.y + 1);
        ImVec2 inner_max = ImVec2(box_max.x - 1, box_max.y - 1);

        if (*value) {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::accent_color));
        } else {
            draw_list->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(gui::theme::background_color));
            draw_list->AddRect(inner_min, inner_max, ImGui::GetColorU32(gui::theme::inline_color));
        }

        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x + 15.0f, pos.y + 1.5f), ImGui::GetColorU32(gui::theme::text_color));

        draw_list->AddRect(btn_bb.Min, btn_bb.Max, ImGui::GetColorU32(gui::theme::outline_color));
        draw_list->AddRect(ImVec2(btn_bb.Min.x + 1, btn_bb.Min.y + 1), ImVec2(btn_bb.Max.x - 1, btn_bb.Max.y - 1), ImGui::GetColorU32(gui::theme::inline_color));
        draw_list->AddRectFilled(ImVec2(btn_bb.Min.x + 2, btn_bb.Min.y + 2), ImVec2(btn_bb.Max.x - 2, btn_bb.Max.y - 2), ImGui::GetColorU32(gui::theme::background_color));

        ImU32 btn_text_col = btn_hovered ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
        float btn_text_x = btn_bb.Min.x + (btn_w - text_size.x) * 0.5f;
        float btn_text_y = btn_bb.Min.y + (btn_h - text_size.y) * 0.5f - 2.5f;
        gui::theme::draw_text_stroke(draw_list, btn_text, ImVec2(btn_text_x, btn_text_y), btn_text_col);

        ImGui::SetNextWindowSize(ImVec2(popup_width, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, gui::theme::background_color);
        ImGui::PushStyleColor(ImGuiCol_Border, gui::theme::outline_color);

        if (ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImVec2 wpos = ImGui::GetWindowPos();
            float pop_actual_w = ImGui::GetWindowWidth();
            ImDrawList* pdl = ImGui::GetWindowDrawList();
            ImVec2 pop_mouse = ImGui::GetIO().MousePos;

            gui::theme::draw_accent_bar(pdl, ImVec2(wpos.x + 1, wpos.y + 1), ImVec2(wpos.x + pop_actual_w - 1, wpos.y + 3), ImGui::GetColorU32(gui::theme::accent_color));
            pdl->AddLine(ImVec2(wpos.x, wpos.y + 3), ImVec2(wpos.x + pop_actual_w, wpos.y + 3), ImGui::GetColorU32(gui::theme::outline_color));

            const char* header_title = (title && title[0]) ? title : label;
            gui::theme::draw_text_stroke(pdl, header_title, ImVec2(wpos.x + 6, wpos.y + 5), ImGui::GetColorU32(gui::theme::text_color));

            ImVec2 close_min = ImVec2(wpos.x + pop_actual_w - 16, wpos.y + 4);
            ImVec2 close_max = ImVec2(wpos.x + pop_actual_w - 4, wpos.y + 16);
            bool close_hov = ImRect(close_min, close_max).Contains(pop_mouse);
            ImU32 close_col = close_hov ? IM_COL32(255, 255, 255, 255) : ImGui::GetColorU32(gui::theme::text_dim_color);
            gui::theme::draw_text_stroke(pdl, "x", ImVec2(close_min.x + 2, close_min.y - 1), close_col);
            if (close_hov && ImGui::IsMouseClicked(0)) {
                ImGui::CloseCurrentPopup();
            }

            pdl->AddLine(ImVec2(wpos.x, wpos.y + 18), ImVec2(wpos.x + pop_actual_w, wpos.y + 18), ImGui::GetColorU32(gui::theme::outline_color));

            ImGui::Dummy(ImVec2(0, 16.0f));

            if (render_fn) {
                render_fn();
            }

            ImGui::Dummy(ImVec2(0, 3.0f));

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        return clicked;
    }

    bool toggle_with_settings(const char* label, bool* value, unsigned int* flags, const char* const items[], int item_count) {
        return toggle_with_settings(label, value, label, [&]() {
            multi_dropdown("Options", flags, items, item_count);
        }, 160.0f);
    }

    bool slider_float(const char* label, float* value, float min_val, float max_val, const char* suffix, int decimal) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float total_height = 26.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + total_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        ImDrawList* draw_list = window->DrawList;
        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool widget_hovered = bb.Contains(mouse);

        bool changed = false;

        
        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x, pos.y), ImGui::GetColorU32(gui::theme::text_color));

        
        if (widget_hovered) {
            float btn_w  = 12.0f;
            float btn_h  = 12.0f;
            float gap    = 4.0f;
            float step   = (decimal == 0) ? 1.0f : 0.1f;

            ImVec2 plus_min  = ImVec2(pos.x + width - btn_w, pos.y);
            ImVec2 plus_max  = ImVec2(pos.x + width, pos.y + btn_h);
            ImVec2 minus_min = ImVec2(pos.x + width - btn_w * 2.0f - gap, pos.y);
            ImVec2 minus_max = ImVec2(pos.x + width - btn_w - gap, pos.y + btn_h);

            bool plus_hov  = ImRect(plus_min,  plus_max).Contains(mouse);
            bool minus_hov = ImRect(minus_min, minus_max).Contains(mouse);

            bool plus_pressed  = plus_hov  && ImGui::IsMouseDown(0);
            bool minus_pressed = minus_hov && ImGui::IsMouseDown(0);

            if (plus_hov  && ImGui::IsMouseClicked(0)) { *value = std::clamp(*value + step, min_val, max_val); changed = true; }
            if (minus_hov && ImGui::IsMouseClicked(0)) { *value = std::clamp(*value - step, min_val, max_val); changed = true; }

            
            ImU32 minus_col = (minus_hov || minus_pressed) ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
            ImVec2 minus_tsz = ImGui::CalcTextSize("-");
            gui::theme::draw_text_stroke(draw_list, "-", ImVec2(minus_min.x + (btn_w - minus_tsz.x) * 0.5f, minus_min.y + (btn_h - minus_tsz.y) * 0.5f - 1.0f), minus_col);

            
            ImU32 plus_col = (plus_hov || plus_pressed) ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
            ImVec2 plus_tsz = ImGui::CalcTextSize("+");
            gui::theme::draw_text_stroke(draw_list, "+", ImVec2(plus_min.x + (btn_w - plus_tsz.x) * 0.5f, plus_min.y + (btn_h - plus_tsz.y) * 0.5f - 1.0f), plus_col);
        }

        
        ImVec2 bar_min = ImVec2(pos.x, pos.y + 14);
        ImVec2 bar_max = ImVec2(pos.x + width, pos.y + 25);

        ImRect bar_bb(bar_min, bar_max);
        bool hovered = ImGui::ItemHoverable(bar_bb, id, 0);

        if (ImGui::IsItemActive()) {
            if (ImGui::IsMouseDown(0)) {
                float mouse_x = ImGui::GetIO().MousePos.x;
                float norm = std::clamp((mouse_x - bar_min.x) / width, 0.0f, 1.0f);
                float new_val = min_val + norm * (max_val - min_val);
                if (new_val != *value) {
                    *value = new_val;
                    changed = true;
                }
            } else {
                ImGui::ClearActiveID();
            }
        } else if (hovered && ImGui::IsMouseClicked(0)) {
            ImGui::SetActiveID(id, window);
            float mouse_x = ImGui::GetIO().MousePos.x;
            float norm = std::clamp((mouse_x - bar_min.x) / width, 0.0f, 1.0f);
            float new_val = min_val + norm * (max_val - min_val);
            if (new_val != *value) {
                *value = new_val;
                changed = true;
            }
        }

        *value = std::clamp(*value, min_val, max_val);

        
        draw_list->AddRect(bar_min, bar_max, ImGui::GetColorU32(gui::theme::outline_color));

        
        ImVec2 fill_min = ImVec2(bar_min.x + 1, bar_min.y + 1);
        ImVec2 fill_max = ImVec2(bar_max.x - 1, bar_max.y - 1);
        draw_list->AddRectFilled(fill_min, fill_max, IM_COL32(34, 34, 34, 255));

        
        float progress = std::clamp((*value - min_val) / (max_val - min_val), 0.0f, 1.0f);
        float fill_w = (width - 2.0f) * progress;
        if (fill_w > 0.0f) {
            gui::theme::draw_accent_bar(draw_list, fill_min, ImVec2(fill_min.x + fill_w, fill_max.y), ImGui::GetColorU32(gui::theme::accent_color));
        }

        
        char val_str[32];
        const char* actual_suffix = (suffix && strcmp(suffix, "%.1f") != 0 && strcmp(suffix, "f") != 0 && strcmp(suffix, "%") != 0) ? suffix : "";
        float diff = std::fabs(*value - std::round(*value));
        if (decimal == 0 || diff < 0.001f) {
            snprintf(val_str, sizeof(val_str), "%d%s", static_cast<int>(std::round(*value)), actual_suffix);
        } else {
            snprintf(val_str, sizeof(val_str), "%.1f%s", *value, actual_suffix);
        }
        ImVec2 val_size = ImGui::CalcTextSize(val_str);
        ImVec2 val_pos = ImVec2(bar_min.x + (width - val_size.x) * 0.5f, bar_min.y + (11.0f - val_size.y) * 0.5f);
        gui::theme::draw_text_stroke(draw_list, val_str, val_pos, ImGui::GetColorU32(gui::theme::text_color));

        return changed;
    }

    bool slider_int(const char* label, int* value, int min, int max, const char* suffix) {
        float f = (float)*value;
        bool changed = slider_float(label, &f, (float)min, (float)max, suffix, 0);
        if (changed) {
            *value = (int)f;
        }
        return changed;
    }

    bool dropdown(const char* label, int* current_item, const char* const items[], int item_count) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float total_height = 32.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + total_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        ImDrawList* draw_list = window->DrawList;

        
        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x, pos.y), ImGui::GetColorU32(gui::theme::text_color));

        
        ImVec2 box_min = ImVec2(pos.x, pos.y + 14);
        ImVec2 box_max = ImVec2(pos.x + width, pos.y + 32);

        ImRect box_bb(box_min, box_max);
        bool hovered = ImGui::ItemHoverable(box_bb, id, 0);
        bool clicked = hovered && ImGui::IsMouseClicked(0);

        char popup_id[64];
        snprintf(popup_id, sizeof(popup_id), "##pop_%s", label);

        if (clicked) {
            ImGui::OpenPopup(popup_id);
        }

        
        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));

        
        draw_list->AddRect(ImVec2(box_min.x + 1, box_min.y + 1), ImVec2(box_max.x - 1, box_max.y - 1), ImGui::GetColorU32(gui::theme::inline_color));

        
        gui::theme::draw_gradient_v(
            draw_list,
            ImVec2(box_min.x + 2, box_min.y + 2),
            ImVec2(box_max.x - 2, box_max.y - 2),
            IM_COL32(42, 42, 42, 255),
            IM_COL32(32, 32, 32, 255)
        );

        
        const char* preview_raw = (*current_item >= 0 && *current_item < item_count) ? items[*current_item] : "None";
        std::string display_preview = preview_raw;
        float max_text_w = width - 26.0f;
        ImVec2 preview_sz = ImGui::CalcTextSize(display_preview.c_str());
        if (preview_sz.x > max_text_w && display_preview.length() > 3) {
            while (display_preview.length() > 3 && ImGui::CalcTextSize((display_preview + "...").c_str()).x > max_text_w) {
                display_preview.pop_back();
            }
            display_preview += "...";
            preview_sz = ImGui::CalcTextSize(display_preview.c_str());
        }
        gui::theme::draw_text_stroke(draw_list, display_preview.c_str(), ImVec2(box_min.x + 6, box_min.y + (18.0f - preview_sz.y) * 0.5f - 0.5f), ImGui::GetColorU32(gui::theme::text_color));

        
        ImVec2 icon_sz = ImGui::CalcTextSize("-");
        gui::theme::draw_text_stroke(draw_list, "-", ImVec2(box_max.x - icon_sz.x - 6, box_min.y + (18.0f - icon_sz.y) * 0.5f - 0.5f), ImGui::GetColorU32(gui::theme::text_dim_color));

        
        bool value_changed = false;
        ImGui::SetNextWindowPos(ImVec2(box_min.x, box_max.y + 1));
        ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width, 180.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 3));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, gui::theme::background_color);
        ImGui::PushStyleColor(ImGuiCol_Border, gui::theme::outline_color);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, gui::theme::accent_color);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, gui::theme::accent_color);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, gui::theme::accent_color);

        if (ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove)) {
            for (int i = 0; i < item_count; i++) {
                bool is_selected = (*current_item == i);

                ImVec2 item_pos = ImGui::GetCursorScreenPos();
                ImVec2 item_sz  = ImGui::CalcTextSize(items[i]);
                float item_w    = ImGui::GetContentRegionAvail().x;
                float item_h    = item_sz.y + 4.0f;

                ImVec2 item_max = ImVec2(item_pos.x + item_w, item_pos.y + item_h);
                bool item_hov   = ImRect(item_pos, item_max).Contains(ImGui::GetIO().MousePos);

                
                if (item_hov) {
                    ImGui::GetWindowDrawList()->AddRectFilled(item_pos, item_max, IM_COL32(45, 45, 45, 255));
                }

                
                ImU32 text_col;
                if (is_selected)     text_col = ImGui::GetColorU32(gui::theme::accent_color);
                else                 text_col = ImGui::GetColorU32(gui::theme::text_color);

                gui::theme::draw_text_stroke(ImGui::GetWindowDrawList(), items[i],
                    ImVec2(item_pos.x + 2, item_pos.y + (item_h - item_sz.y) * 0.5f), text_col);

                ImGui::Dummy(ImVec2(item_w, item_h));

                if (item_hov && ImGui::IsMouseClicked(0)) {
                    *current_item = i;
                    value_changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);

        return value_changed;
    }

    bool multi_dropdown(const char* label, unsigned int* flags, const char* const items[], int item_count) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();
        float total_height = 32.0f;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + total_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        ImDrawList* draw_list = window->DrawList;

        
        gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x, pos.y), ImGui::GetColorU32(gui::theme::text_color));

        
        ImVec2 box_min = ImVec2(pos.x, pos.y + 14);
        ImVec2 box_max = ImVec2(pos.x + width, pos.y + 32);

        ImRect box_bb(box_min, box_max);
        bool hovered = ImGui::ItemHoverable(box_bb, id, 0);
        bool clicked = hovered && ImGui::IsMouseClicked(0);

        char popup_id[64];
        snprintf(popup_id, sizeof(popup_id), "##mpop_%s", label);

        if (clicked) {
            ImGui::OpenPopup(popup_id);
        }

        
        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));
        draw_list->AddRect(ImVec2(box_min.x + 1, box_min.y + 1), ImVec2(box_max.x - 1, box_max.y - 1), ImGui::GetColorU32(gui::theme::inline_color));
        
        gui::theme::draw_gradient_v(
            draw_list,
            ImVec2(box_min.x + 2, box_min.y + 2),
            ImVec2(box_max.x - 2, box_max.y - 2),
            IM_COL32(42, 42, 42, 255),
            IM_COL32(32, 32, 32, 255)
        );

        
        std::string preview = "";
        int count = 0;
        for (int i = 0; i < item_count; i++) {
            if (*flags & (1 << i)) {
                if (count > 0) preview += ", ";
                preview += items[i];
                count++;
            }
        }
        if (count == 0) preview = "None";

        float max_text_w = width - 36.0f;
        std::string display_preview = preview;
        ImVec2 preview_sz = ImGui::CalcTextSize(display_preview.c_str());
        if (preview_sz.x > max_text_w && display_preview.length() > 3) {
            while (display_preview.length() > 3 && ImGui::CalcTextSize((display_preview + "...").c_str()).x > max_text_w) {
                display_preview.pop_back();
            }
            display_preview += "...";
            preview_sz = ImGui::CalcTextSize(display_preview.c_str());
        }

        gui::theme::draw_text_stroke(draw_list, display_preview.c_str(), ImVec2(box_min.x + 6, box_min.y + (18.0f - preview_sz.y) * 0.5f - 0.5f), ImGui::GetColorU32(gui::theme::text_color));

        
        ImVec2 icon_sz = ImGui::CalcTextSize("...");
        gui::theme::draw_text_stroke(draw_list, "...", ImVec2(box_max.x - icon_sz.x - 6, box_min.y + (18.0f - icon_sz.y) * 0.5f - 0.5f), ImGui::GetColorU32(gui::theme::text_dim_color));

        bool changed = false;
        ImGui::SetNextWindowPos(ImVec2(box_min.x, box_max.y + 1));
        ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width, 180.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, gui::theme::background_color);
        ImGui::PushStyleColor(ImGuiCol_Border, gui::theme::outline_color);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, gui::theme::accent_color);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, gui::theme::accent_color);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, gui::theme::accent_color);

        if (ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove)) {
            for (int i = 0; i < item_count; i++) {
                bool is_checked = (*flags & (1 << i)) != 0;

                ImVec2 item_pos = ImGui::GetCursorScreenPos();
                ImVec2 item_sz  = ImGui::CalcTextSize(items[i]);
                float item_w    = ImGui::GetContentRegionAvail().x;
                float item_h    = item_sz.y + 4.0f;

                ImVec2 item_max = ImVec2(item_pos.x + item_w, item_pos.y + item_h);
                bool item_hov   = ImRect(item_pos, item_max).Contains(ImGui::GetIO().MousePos);

                if (item_hov) {
                    ImGui::GetWindowDrawList()->AddRectFilled(item_pos, item_max, IM_COL32(45, 45, 45, 255));
                }

                ImU32 text_col;
                if (is_checked)    text_col = ImGui::GetColorU32(gui::theme::accent_color);
                else               text_col = ImGui::GetColorU32(gui::theme::text_color);

                gui::theme::draw_text_stroke(ImGui::GetWindowDrawList(), items[i],
                    ImVec2(item_pos.x + 4.0f, item_pos.y + (item_h - item_sz.y) * 0.5f), text_col);

                ImGui::Dummy(ImVec2(item_w, item_h));

                if (item_hov && ImGui::IsMouseClicked(0)) {
                    *flags ^= (1 << i);
                    changed = true;
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);

        return changed;
    }

    bool colorpicker(const char* id, float color[4], const char* title) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID widget_id = window->GetID(id);
        ImVec2 pos = window->DC.CursorPos;

        
        ImVec2 size = ImVec2(20, 9);
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, widget_id))
            return false;

        bool hovered = ImGui::ItemHoverable(bb, widget_id, 0);
        bool clicked = hovered && ImGui::IsMouseClicked(0);

        char popup_id[64];
        snprintf(popup_id, sizeof(popup_id), "##cppop_%08X", (unsigned int)widget_id);

        static ImGuiID active_picker_id = 0;
        static float old_color[4] = { 0 };
        static float cur_h = 0.0f, cur_s = 0.0f, cur_v = 0.0f, cur_a = 1.0f;
        static int active_drag = 0; 

        if (clicked) {
            active_picker_id = widget_id;
            old_color[0] = color[0];
            old_color[1] = color[1];
            old_color[2] = color[2];
            old_color[3] = color[3];
            ImGui::ColorConvertRGBtoHSV(color[0], color[1], color[2], cur_h, cur_s, cur_v);
            cur_a = color[3];
            active_drag = 0;
            ImGui::OpenPopup(popup_id);
        }

        ImDrawList* draw_list = window->DrawList;

        
        draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(gui::theme::outline_color));

        
        ImU32 col32 = ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
        draw_list->AddRectFilled(ImVec2(bb.Min.x + 1, bb.Min.y + 1), ImVec2(bb.Max.x - 1, bb.Max.y - 1), col32);

        bool changed = false;
        const float pop_w = 257.0f;
        const float pop_h = 176.0f;

        ImGui::SetNextWindowSize(ImVec2(pop_w, pop_h));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, gui::theme::background_color);
        ImGui::PushStyleColor(ImGuiCol_Border, gui::theme::outline_color);

        if (ImGui::BeginPopup(popup_id, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            ImVec2 wpos = ImGui::GetWindowPos();
            ImDrawList* pdl = ImGui::GetWindowDrawList();
            ImVec2 mouse = ImGui::GetIO().MousePos;

            
            gui::theme::draw_accent_bar(pdl, ImVec2(wpos.x + 1, wpos.y + 1), ImVec2(wpos.x + pop_w - 1, wpos.y + 3), ImGui::GetColorU32(gui::theme::accent_color));
            pdl->AddLine(ImVec2(wpos.x, wpos.y + 3), ImVec2(wpos.x + pop_w, wpos.y + 3), ImGui::GetColorU32(gui::theme::outline_color));

            
            const char* header_title = (title && title[0]) ? title : "Color Picker";
            gui::theme::draw_text_stroke(pdl, header_title, ImVec2(wpos.x + 6, wpos.y + 5), ImGui::GetColorU32(gui::theme::text_color));

            
            ImVec2 close_min = ImVec2(wpos.x + pop_w - 16, wpos.y + 4);
            ImVec2 close_max = ImVec2(wpos.x + pop_w - 4, wpos.y + 16);
            bool close_hov = ImRect(close_min, close_max).Contains(mouse);
            ImU32 close_col = close_hov ? IM_COL32(255, 255, 255, 255) : ImGui::GetColorU32(gui::theme::text_dim_color);
            gui::theme::draw_text_stroke(pdl, "x", ImVec2(close_min.x + 2, close_min.y - 1), close_col);
            if (close_hov && ImGui::IsMouseClicked(0)) {
                ImGui::CloseCurrentPopup();
            }

            
            ImVec2 sv_min = ImVec2(wpos.x + 8.0f, wpos.y + 22.0f);
            ImVec2 sv_max = ImVec2(sv_min.x + 145.0f, sv_min.y + 120.0f);
            float sv_w = sv_max.x - sv_min.x;
            float sv_h = sv_max.y - sv_min.y;

            
            ImVec2 hue_min = ImVec2(sv_max.x + 8.0f, sv_min.y);
            ImVec2 hue_max = ImVec2(hue_min.x + 10.0f, sv_max.y);
            float hue_h = hue_max.y - hue_min.y;

            
            ImVec2 alpha_min = ImVec2(sv_min.x, sv_max.y + 7.0f);
            ImVec2 alpha_max = ImVec2(sv_max.x, alpha_min.y + 10.0f);
            float alpha_w = alpha_max.x - alpha_min.x;

            if (!ImGui::IsMouseDown(0)) {
                active_drag = 0;
            } else if (ImGui::IsMouseClicked(0)) {
                if (ImRect(sv_min, sv_max).Contains(mouse)) active_drag = 1;
                else if (ImRect(hue_min, hue_max).Contains(mouse)) active_drag = 2;
                else if (ImRect(alpha_min, alpha_max).Contains(mouse)) active_drag = 3;
            }

            if (active_drag == 1) {
                cur_s = std::clamp((mouse.x - sv_min.x) / sv_w, 0.0f, 1.0f);
                cur_v = std::clamp(1.0f - (mouse.y - sv_min.y) / sv_h, 0.0f, 1.0f);
            } else if (active_drag == 2) {
                cur_h = std::clamp((mouse.y - hue_min.y) / hue_h, 0.0f, 1.0f);
                if (cur_h >= 1.0f) cur_h = 0.999f;
            } else if (active_drag == 3) {
                cur_a = std::clamp((mouse.x - alpha_min.x) / alpha_w, 0.0f, 1.0f);
            }

            
            float nr = 0, ng = 0, nb = 0;
            ImGui::ColorConvertHSVtoRGB(cur_h, cur_s, cur_v, nr, ng, nb);

            
            float hr = 0, hg = 0, hb = 0;
            ImGui::ColorConvertHSVtoRGB(cur_h, 1.0f, 1.0f, hr, hg, hb);
            ImU32 hue_pure = IM_COL32((int)(hr * 255), (int)(hg * 255), (int)(hb * 255), 255);
            ImU32 white_col = IM_COL32(255, 255, 255, 255);
            pdl->AddRectFilledMultiColor(sv_min, sv_max, white_col, hue_pure, hue_pure, white_col);

            ImU32 trans_black = IM_COL32(0, 0, 0, 0);
            ImU32 full_black  = IM_COL32(0, 0, 0, 255);
            pdl->AddRectFilledMultiColor(sv_min, sv_max, trans_black, trans_black, full_black, full_black);

            pdl->AddRect(sv_min, sv_max, ImGui::GetColorU32(gui::theme::outline_color));

            
            ImVec2 sv_cur = ImVec2(sv_min.x + cur_s * sv_w, sv_min.y + (1.0f - cur_v) * sv_h);
            pdl->AddRect(ImVec2(sv_cur.x - 2, sv_cur.y - 2), ImVec2(sv_cur.x + 3, sv_cur.y + 3), IM_COL32(0, 0, 0, 255));
            pdl->AddRectFilled(ImVec2(sv_cur.x - 1, sv_cur.y - 1), ImVec2(sv_cur.x + 2, sv_cur.y + 2), IM_COL32(255, 255, 255, 255));

            
            const ImColor rainbow_cols[] = {
                ImColor(255, 0, 0),
                ImColor(255, 255, 0),
                ImColor(0, 255, 0),
                ImColor(0, 255, 255),
                ImColor(0, 0, 255),
                ImColor(255, 0, 255),
                ImColor(255, 0, 0)
            };
            float slice_h = hue_h / 6.0f;
            for (int s = 0; s < 6; s++) {
                ImVec2 s_min = ImVec2(hue_min.x, hue_min.y + s * slice_h);
                ImVec2 s_max = ImVec2(hue_max.x, hue_min.y + (s + 1) * slice_h);
                ImU32 c_top = rainbow_cols[s];
                ImU32 c_bot = rainbow_cols[s + 1];
                pdl->AddRectFilledMultiColor(s_min, s_max, c_top, c_top, c_bot, c_bot);
            }
            pdl->AddRect(hue_min, hue_max, ImGui::GetColorU32(gui::theme::outline_color));

            
            float h_pos_y = hue_min.y + cur_h * hue_h;
            pdl->AddRect(ImVec2(hue_min.x - 1, h_pos_y - 1), ImVec2(hue_max.x + 1, h_pos_y + 2), IM_COL32(0, 0, 0, 255));
            pdl->AddRectFilled(ImVec2(hue_min.x, h_pos_y), ImVec2(hue_max.x, h_pos_y + 1), IM_COL32(255, 255, 255, 255));

            
            pdl->AddRectFilledMultiColor(alpha_min, alpha_max, IM_COL32(10, 10, 10, 255), IM_COL32(255, 255, 255, 255), IM_COL32(255, 255, 255, 255), IM_COL32(10, 10, 10, 255));
            pdl->AddRect(alpha_min, alpha_max, ImGui::GetColorU32(gui::theme::outline_color));

            
            float a_pos_x = alpha_min.x + cur_a * alpha_w;
            pdl->AddRect(ImVec2(a_pos_x - 1, alpha_min.y - 1), ImVec2(a_pos_x + 2, alpha_max.y + 1), IM_COL32(0, 0, 0, 255));
            pdl->AddRectFilled(ImVec2(a_pos_x, alpha_min.y), ImVec2(a_pos_x + 1, alpha_max.y), IM_COL32(255, 255, 255, 255));

            
            float right_col_x = hue_max.x + 6.0f;
            float preview_w = 78.0f;
            float preview_h = 24.0f;

            
            gui::theme::draw_text_stroke(pdl, "New Color", ImVec2(right_col_x, sv_min.y), ImGui::GetColorU32(gui::theme::text_color));
            ImVec2 new_box_min = ImVec2(right_col_x, sv_min.y + 15.0f);
            ImVec2 new_box_max = ImVec2(right_col_x + preview_w, new_box_min.y + preview_h);
            pdl->AddRect(new_box_min, new_box_max, ImGui::GetColorU32(gui::theme::outline_color));
            pdl->AddRectFilled(ImVec2(new_box_min.x + 1, new_box_min.y + 1), ImVec2(new_box_max.x - 1, new_box_max.y - 1),
                IM_COL32((int)(nr*255), (int)(ng*255), (int)(nb*255), (int)(cur_a*255)));

            
            gui::theme::draw_text_stroke(pdl, "Old Color", ImVec2(right_col_x, new_box_max.y + 8.0f), ImGui::GetColorU32(gui::theme::text_color));
            ImVec2 old_box_min = ImVec2(right_col_x, new_box_max.y + 23.0f);
            ImVec2 old_box_max = ImVec2(right_col_x + preview_w, old_box_min.y + preview_h);
            pdl->AddRect(old_box_min, old_box_max, ImGui::GetColorU32(gui::theme::outline_color));
            pdl->AddRectFilled(ImVec2(old_box_min.x + 1, old_box_min.y + 1), ImVec2(old_box_max.x - 1, old_box_max.y - 1),
                IM_COL32((int)(old_color[0]*255), (int)(old_color[1]*255), (int)(old_color[2]*255), (int)(old_color[3]*255)));

            if (ImRect(old_box_min, old_box_max).Contains(mouse) && ImGui::IsMouseClicked(0)) {
                ImGui::ColorConvertRGBtoHSV(old_color[0], old_color[1], old_color[2], cur_h, cur_s, cur_v);
                cur_a = old_color[3];
            }

            
            ImVec2 apply_sz = ImGui::CalcTextSize("[ Apply ]");
            ImVec2 apply_pos = ImVec2(right_col_x + (preview_w - apply_sz.x) * 0.5f, alpha_min.y - 1.0f);
            ImRect apply_bb(ImVec2(apply_pos.x - 4.0f, apply_pos.y - 2.0f), ImVec2(apply_pos.x + apply_sz.x + 4.0f, apply_pos.y + apply_sz.y + 4.0f));
            bool apply_hov = apply_bb.Contains(mouse);
            ImU32 apply_col = apply_hov ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
            gui::theme::draw_text_stroke(pdl, "[ Apply ]", apply_pos, apply_col);

            if (apply_hov && ImGui::IsMouseClicked(0)) {
                color[0] = nr; color[1] = ng; color[2] = nb; color[3] = cur_a;
                old_color[0] = color[0]; old_color[1] = color[1]; old_color[2] = color[2]; old_color[3] = color[3];
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        return changed;
    }

    bool keybind(const char* id, int* key, int* mode) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID widget_id = window->GetID(id);
        ImVec2 pos = window->DC.CursorPos;

        char label_buf[32];
        if ((int)widget_id == active_keybind_id) {
            snprintf(label_buf, sizeof(label_buf), "...");
        } else {
            snprintf(label_buf, sizeof(label_buf), "%s", get_key_name(*key));
        }

        ImVec2 text_size = ImGui::CalcTextSize(label_buf);
        ImVec2 size = ImVec2(text_size.x + 8.0f, 15.0f);

        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, widget_id))
            return false;

        bool hovered = ImGui::ItemHoverable(bb, widget_id, 0);
        if (hovered && ImGui::IsMouseClicked(0)) {
            if (active_keybind_id == (int)widget_id) {
                active_keybind_id = -1;
                keybind_waiting_release = false;
            } else {
                active_keybind_id = (int)widget_id;
                keybind_waiting_release = true;
            }
        }

        if ((int)widget_id == active_keybind_id) {
            if (keybind_waiting_release) {
                if (!ImGui::IsMouseDown(0) && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
                    keybind_waiting_release = false;
                }
            } else {
                for (int k = 1; k < 255; k++) {
                    if (GetAsyncKeyState(k) & 0x8000) {
                        if (k == VK_ESCAPE) {
                            *key = 0;
                        } else {
                            *key = k;
                        }
                        active_keybind_id = -1;
                        break;
                    }
                }
            }
        }

        char mode_popup_id[64];
        snprintf(mode_popup_id, sizeof(mode_popup_id), "##kbmode_%s", id);

        if (hovered && ImGui::IsMouseClicked(1)) {
            ImGui::OpenPopup(mode_popup_id);
        }

        ImDrawList* draw_list = window->DrawList;

        
        draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(gui::theme::outline_color));
        draw_list->AddRect(ImVec2(bb.Min.x + 1, bb.Min.y + 1), ImVec2(bb.Max.x - 1, bb.Max.y - 1), ImGui::GetColorU32(gui::theme::inline_color));
        draw_list->AddRectFilled(ImVec2(bb.Min.x + 2, bb.Min.y + 2), ImVec2(bb.Max.x - 2, bb.Max.y - 2), ImGui::GetColorU32(gui::theme::background_color));

        ImU32 text_col = ((int)widget_id == active_keybind_id || hovered) ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
        float text_y = bb.Min.y + (15.0f - text_size.y) * 0.5f - 0.5f;
        gui::theme::draw_text_stroke(draw_list, label_buf, ImVec2(bb.Min.x + 4.0f, text_y), text_col);

        ImGui::SetNextWindowSize(ImVec2(80.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, gui::theme::background_color);
        ImGui::PushStyleColor(ImGuiCol_Border, gui::theme::outline_color);

        if (ImGui::BeginPopup(mode_popup_id, ImGuiWindowFlags_NoMove)) {
            
            
            struct ModeItem { const char* name; int val; };
            static const ModeItem mode_items[] = {
                { "Hold",     1 },
                { "Toggle",   2 },
                { "Hold Off", 3 },
                { "Always",   0 },
            };
            for (const auto& mi : mode_items) {
                bool is_active = (*mode == mi.val);
                ImU32 col = is_active ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
                ImVec2 item_pos = ImGui::GetCursorScreenPos();
                ImVec2 item_sz = ImGui::CalcTextSize(mi.name);
                float item_w = ImGui::GetContentRegionAvail().x;

                
                ImVec2 item_max = ImVec2(item_pos.x + item_w, item_pos.y + item_sz.y + 2.0f);
                bool item_hov = ImRect(item_pos, item_max).Contains(ImGui::GetIO().MousePos);
                if (item_hov) col = ImGui::GetColorU32(gui::theme::accent_color);

                ImDrawList* pd = ImGui::GetWindowDrawList();
                gui::theme::draw_text_stroke(pd, mi.name, ImVec2(item_pos.x, item_pos.y + 1.0f), col);
                ImGui::Dummy(ImVec2(item_w, item_sz.y + 2.0f));

                if (item_hov && ImGui::IsMouseClicked(0)) {
                    *mode = mi.val;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        return true;
    }

    bool button(const char* label, float width) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float w = (width > 0.0f) ? width : get_widget_width();
        float h = 18.0f; 

        ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered = ImGui::ItemHoverable(bb, id, 0);
        bool clicked = hovered && ImGui::IsMouseClicked(0);

        ImDrawList* draw_list = window->DrawList;

        
        draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(gui::theme::outline_color));

        
        draw_list->AddRect(ImVec2(bb.Min.x + 1, bb.Min.y + 1), ImVec2(bb.Max.x - 1, bb.Max.y - 1), ImGui::GetColorU32(gui::theme::inline_color));

        
        ImU32 top_col = hovered ? IM_COL32(50, 50, 50, 255) : IM_COL32(40, 40, 40, 255);
        ImU32 bot_col = hovered ? IM_COL32(38, 38, 38, 255) : IM_COL32(32, 32, 32, 255);
        if (ImGui::IsMouseDown(0) && hovered) {
            top_col = IM_COL32(30, 30, 30, 255);
            bot_col = IM_COL32(24, 24, 24, 255);
        }

        gui::theme::draw_gradient_v(draw_list, ImVec2(bb.Min.x + 2, bb.Min.y + 2), ImVec2(bb.Max.x - 2, bb.Max.y - 2), top_col, bot_col);

        
        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImVec2 text_pos = ImVec2(bb.Min.x + (w - text_size.x) * 0.5f, bb.Min.y + (18.0f - text_size.y) * 0.5f - 0.5f);
        gui::theme::draw_text_stroke(draw_list, label, text_pos, ImGui::GetColorU32(gui::theme::text_color));

        return clicked;
    }

    bool textbox(const char* label, char* buffer, size_t buffer_size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        float width = get_widget_width();

        bool has_label = (label && label[0] && !(label[0] == '#' && label[1] == '#'));
        float total_height = has_label ? 32.0f : 18.0f;
        float box_y = has_label ? (pos.y + 14.0f) : pos.y;

        ImRect bb(pos, ImVec2(pos.x + width, pos.y + total_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        ImDrawList* draw_list = window->DrawList;

        if (has_label) {
            gui::theme::draw_text_stroke(draw_list, label, ImVec2(pos.x, pos.y), ImGui::GetColorU32(gui::theme::text_color));
        }

        ImVec2 box_min = ImVec2(pos.x, box_y);
        ImVec2 box_max = ImVec2(pos.x + width, box_y + 18.0f);

        
        draw_list->AddRect(box_min, box_max, ImGui::GetColorU32(gui::theme::outline_color));
        draw_list->AddRect(ImVec2(box_min.x + 1, box_min.y + 1), ImVec2(box_max.x - 1, box_max.y - 1), ImGui::GetColorU32(gui::theme::inline_color));
        draw_list->AddRectFilled(ImVec2(box_min.x + 2, box_min.y + 2), ImVec2(box_max.x - 2, box_max.y - 2), IM_COL32(32, 32, 32, 255));

        
        ImGui::SetCursorScreenPos(ImVec2(box_min.x + 4.0f, box_min.y + 1.0f));
        ImGui::PushItemWidth(width - 8.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));

        char input_id[64];
        snprintf(input_id, sizeof(input_id), "##it_%s", label);
        bool changed = ImGui::InputText(input_id, buffer, buffer_size);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        return changed;
    }

    bool listbox(const char* label, int* current_item, const char* const items[], int item_count, float height) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, gui::theme::tab_bg_color);
        ImGui::PushStyleColor(ImGuiCol_Border, gui::theme::outline_color);

        float width = get_widget_width();
        bool changed = false;

        if (ImGui::BeginListBox(label, ImVec2(width, height))) {
            for (int i = 0; i < item_count; i++) {
                bool is_selected = (*current_item == i);
                ImVec2 item_pos = ImGui::GetCursorScreenPos();
                ImVec2 item_sz = ImGui::CalcTextSize(items[i]);
                float item_w = ImGui::GetContentRegionAvail().x;
                float item_h = item_sz.y + 4.0f;
                ImVec2 item_max = ImVec2(item_pos.x + item_w, item_pos.y + item_h);
                bool item_hov = ImRect(item_pos, item_max).Contains(ImGui::GetIO().MousePos);

                if (is_selected) {
                    ImGui::GetWindowDrawList()->AddRectFilled(item_pos, item_max, IM_COL32(45, 45, 45, 255));
                } else if (item_hov) {
                    ImGui::GetWindowDrawList()->AddRectFilled(item_pos, item_max, IM_COL32(38, 38, 38, 255));
                }

                ImU32 text_col = is_selected ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::text_color);
                gui::theme::draw_text_stroke(ImGui::GetWindowDrawList(), items[i],
                    ImVec2(item_pos.x + 4.0f, item_pos.y + (item_h - item_sz.y) * 0.5f), text_col);

                ImGui::Dummy(ImVec2(item_w, item_h));

                if (item_hov && ImGui::IsMouseClicked(0)) {
                    *current_item = i;
                    changed = true;
                }
            }
            ImGui::EndListBox();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(1);

        return changed;
    }

}

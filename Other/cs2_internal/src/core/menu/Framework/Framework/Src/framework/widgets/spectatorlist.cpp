// Created by Valorr19
// spectatorlist.cpp

#include "../headers/functions.h"
#include "../headers/widgets.h"
#include <algorithm>

void c_spectatorlist::add_spectator(const std::string& name)
{
    for (auto& spec : spectators)
    {
        if (spec.name != name)
            continue;
        spec.active = true;
        return;
    }

    spectator_item item;
    item.name = name;
    item.active = true;
    item.appear_alpha = 0.0f;
    spectators.push_back(item);
}

void c_spectatorlist::remove_spectator(const std::string& name)
{
    for (auto& spec : spectators)
    {
        if (spec.name == name)
            spec.active = false;
    }
}

void c_spectatorlist::clear_spectators()
{
    spectators.clear();
}

void c_spectatorlist::begin_sync()
{
    for (auto& spec : spectators)
        spec.active = false;
}

void c_spectatorlist::end_sync()
{
    spectators.erase(
        std::remove_if(spectators.begin(), spectators.end(),
            [](const spectator_item& s) { return !s.active && s.appear_alpha <= 0.01f; }),
        spectators.end());
}

void c_spectatorlist::render()
{
    if (!gui || !draw || !font)
        return;

    float anim_alpha = 1.0f;
    float scale      = 1.0f;

    try
    {
        ImGuiIO&    io        = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        ImFont* text_font = font->get(main_font_data, 13.0f);
        if (!text_font) return;

        float item_height   = SCALE(18.0f);
        float width         = SCALE(180.0f);

        float visible_count = 0.0f;
        for (auto& spec : spectators)
        {
            gui->easing(spec.appear_alpha, spec.active ? 1.0f : 0.0f, 10.0f, dynamic_easing);
            if (spec.appear_alpha > 0.01f)
                visible_count += spec.appear_alpha;
        }

        if (visible_count <= 0.01f)
            return;

        const bool menu_open = var->gui.menu_open;
        float total_height = visible_count * item_height;

        ImVec2 pos  = spectator_pos;
        ImVec2 size = ImVec2(width, total_height);
        
        ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
        ImVec2 scaled_pos = ImVec2(
            center.x + (pos.x - center.x) * scale,
            center.y + (pos.y - center.y) * scale
        );
        ImVec2 scaled_size = ImVec2(size.x * scale, size.y * scale);

        ImRect panel_rect(scaled_pos, ImVec2(scaled_pos.x + scaled_size.x, scaled_pos.y + scaled_size.y));

        if (menu_open && panel_rect.Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            is_dragging = true;
            drag_offset = ImVec2(ImGui::GetMousePos().x - pos.x,
                                 ImGui::GetMousePos().y - pos.y);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            is_dragging = false;

        if (is_dragging)
        {
            spectator_pos.x = ImClamp(ImGui::GetMousePos().x - drag_offset.x,
                                      0.0f, io.DisplaySize.x - width);
            spectator_pos.y = ImClamp(ImGui::GetMousePos().y - drag_offset.y,
                                      0.0f, io.DisplaySize.y - total_height);
            pos = spectator_pos;
            
            center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
            scaled_pos = ImVec2(
                center.x + (pos.x - center.x) * scale,
                center.y + (pos.y - center.y) * scale
            );
            scaled_size = ImVec2(size.x * scale, size.y * scale);
        }

        float item_y = scaled_pos.y;
        float item_scaled_h = item_height * scale;

        for (int idx = 0; idx < (int)spectators.size(); idx++)
        {
            auto& spec = spectators[idx];
            if (spec.appear_alpha <= 0.01f) continue;

            const float item_a = anim_alpha * spec.appear_alpha;
            const float row_h = item_scaled_h * spec.appear_alpha;

            draw_list->AddText(text_font, 13.f,
                ImVec2(scaled_pos.x, item_y),
                IM_COL32(255, 255, 255, (int)(255 * item_a)), spec.name.c_str());

            item_y += row_h;
        }
    }
    catch (...)
    {
    }
}

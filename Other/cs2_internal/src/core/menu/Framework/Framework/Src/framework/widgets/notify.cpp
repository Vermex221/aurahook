// Created by Valorr19
// notify.cpp

#include "../headers/functions.h"
#include "../headers/widgets.h"

void c_notify::add_notify(std::string_view text, notify_type type)
{
    notifications.push_back({ notify_count++, text, type });
}

void c_notify::setup_notify()
{
    int cur_notify_value = 0;
    float accumulated_height = 0.f;

    notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(),
            [](const notify_state& n) { return n.notify_alpha <= 0.001f && !n.active_notify; }),
        notifications.end()
    );

    for (auto& notification : notifications)
    {
        cur_notify_value++;
        
        if (notification.active_notify)
        {
            notification.notify_timer += 1.0f;  
        }

        if (notification.notify_timer >= notify_time)
        {
            notification.active_notify = false;
        }

        gui->easing(notification.notify_alpha, notification.active_notify ? 1.f : 0.f, 8.f, dynamic_easing);

        if (notification.notify_alpha > 0.001f)
        {
            float target_position = accumulated_height + notify_padding.y;
            gui->easing(notification.notify_pos, target_position, 12.f, dynamic_easing);

            ImVec2 window_size = render_notify(cur_notify_value, notification.notify_alpha, notification.notify_timer, notification.notify_pos, notification.text, notification.type);

            accumulated_height += window_size.y + notify_spacing;
        }
    }
}

ImVec2 c_notify::render_notify(int cur_notify_value, float notify_alpha, float notify_percentage, float notify_pos, std::string_view text, notify_type type)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    
    ImFont* notify_font = font->get(main_font_data, 13.0f);
    if (!notify_font)
        notify_font = ImGui::GetFont();
    
    ImVec2 text_sz = notify_font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, text.data(), text.data() + text.size());
    float width = text_sz.x;
    float height = text_sz.y + SCALE(4.0f);
    
    ImVec2 pos = ImVec2(io.DisplaySize.x - width - notify_padding.x, notify_pos);
    float slide_offset = (1.0f - notify_alpha) * SCALE(40.0f);
    pos.x += slide_offset;
    
    draw_list->AddText(
        notify_font,
        13.0f,
        pos,
        IM_COL32(255, 255, 255, static_cast<int>(255 * notify_alpha)),
        text.data(),
        text.data() + text.size()
    );
    
    return ImVec2(width, height);
}
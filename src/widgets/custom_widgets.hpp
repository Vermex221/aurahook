#pragma once

#include "../imgui/imgui.h"
#include <string>
#include <vector>
#include <functional>

namespace widgets {

    
    bool begin_section(const char* title, float width = 0.0f, float height = 0.0f);
    bool begin_section_subtabs(int* selected_subtab, const char* const subtab_names[], int subtab_count, float width = 0.0f, float height = 0.0f);
    void end_section();

    bool subtabs(int* selected_tab, const char* const tab_names[], int count);
    bool subtab_bar(int* selected_subtab, const char* const subtab_names[], int subtab_count);

    
    bool toggle(const char* label, bool* value);
    bool toggle_with_color(const char* label, bool* value, float color[4], const char* color_title = nullptr);
    bool toggle_with_two_colors(const char* label, bool* value, float color1[4], float color2[4], const char* title1 = nullptr, const char* title2 = nullptr);
    bool toggle_with_keybind(const char* label, bool* value, int* key, int* mode);
    bool toggle_with_settings(const char* label, bool* value, const char* title, const std::function<void()>& render_fn, float popup_width = 160.0f);
    bool toggle_with_settings(const char* label, bool* value, unsigned int* flags, const char* const items[], int item_count);

    
    bool slider_int(const char* label, int* value, int min, int max, const char* suffix = "");
    bool slider_float(const char* label, float* value, float min, float max, const char* suffix = "", int decimal = 1);

    
    bool dropdown(const char* label, int* current_item, const char* const items[], int item_count);
    bool multi_dropdown(const char* label, unsigned int* flags, const char* const items[], int item_count);

    
    bool colorpicker(const char* id, float color[4], const char* title = nullptr);
    bool keybind(const char* id, int* key, int* mode);

    
    bool button(const char* label, float width = -1.0f);
    bool textbox(const char* label, char* buffer, size_t buffer_size);
    bool listbox(const char* label, int* current_item, const char* const items[], int item_count, float height = 130.0f);

    const char* get_key_name(int key);

}

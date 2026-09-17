#pragma once

#include "../../imgui/imgui.h"

namespace gui::theme {

    
    inline ImVec4 outline_color    = ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f);     
    inline ImVec4 inline_color     = ImVec4(50.0f / 255.0f, 50.0f / 255.0f, 50.0f / 255.0f, 1.0f);     
    inline ImVec4 gradient_color   = ImVec4(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f);     
    inline ImVec4 background_color = ImVec4(35.0f / 255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 1.0f);     
    inline ImVec4 tab_bg_color     = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);     
    inline ImVec4 accent_color     = ImVec4(0.0f / 255.0f, 122.0f / 255.0f, 230.0f / 255.0f, 1.0f);   
    inline ImVec4 text_color       = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);                   
    inline ImVec4 text_dim_color   = ImVec4(160.0f / 255.0f, 160.0f / 255.0f, 160.0f / 255.0f, 1.0f); 

    
    inline ImVec4 notif_outline    = ImVec4(52.0f / 255.0f, 52.0f / 255.0f, 52.0f / 255.0f, 1.0f);     
    inline ImVec4 notif_bg         = ImVec4(35.0f / 255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 1.0f);   

    enum class FontID {
        Plex = 0,
        Plex2,
        FSTahoma,
        SmallestPixel,
        Tahoma,
        Minecraftia,
        Count
    };

    inline const char* font_names[] = {
        "Plex",
        "Plex 2",
        "FS Tahoma 8px",
        "Smallest Pixel",
        "Tahoma",
        "Minecraftia"
    };

    inline ImFont* fonts[(int)FontID::Count] = { nullptr };
    inline ImFont* fonts_tabs[(int)FontID::Count] = { nullptr };

    inline ImFont* font_main = nullptr;

    void apply();
    void reset();

    void draw_text(ImDrawList* draw_list, const char* text, const ImVec2& pos, ImU32 col = 0);
    void draw_text_stroke(ImDrawList* draw_list, const char* text, const ImVec2& pos, ImU32 col = 0);
    void draw_accent_bar(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 accent_col);
    void draw_gradient_v(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 col_top, ImU32 col_bot);

}


#include "theme.hpp"

namespace gui::theme {

    void apply() {
        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding    = 0.0f;
        style.ChildRounding     = 0.0f;
        style.FrameRounding     = 0.0f;
        style.PopupRounding     = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding      = 0.0f;
        style.TabRounding       = 0.0f;

        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.FrameBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;

        style.WindowPadding     = ImVec2(8.0f, 8.0f);
        style.FramePadding      = ImVec2(6.0f, 3.0f);
        style.ItemSpacing       = ImVec2(0.0f, 6.0f);
        style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
        style.ScrollbarSize     = 4.0f;
        style.GrabMinSize       = 4.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                  = text_color;
        colors[ImGuiCol_TextDisabled]          = text_dim_color;
        colors[ImGuiCol_WindowBg]              = background_color;
        colors[ImGuiCol_ChildBg]               = gradient_color;
        colors[ImGuiCol_PopupBg]               = background_color;
        colors[ImGuiCol_Border]                = outline_color;
        colors[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
        colors[ImGuiCol_FrameBg]               = gradient_color;
        colors[ImGuiCol_FrameBgHovered]        = inline_color;
        colors[ImGuiCol_FrameBgActive]         = accent_color;
        colors[ImGuiCol_TitleBg]               = outline_color;
        colors[ImGuiCol_TitleBgActive]         = outline_color;
        colors[ImGuiCol_TitleBgCollapsed]      = outline_color;
        colors[ImGuiCol_MenuBarBg]             = outline_color;
        colors[ImGuiCol_ScrollbarBg]           = outline_color;
        colors[ImGuiCol_ScrollbarGrab]         = accent_color;
        colors[ImGuiCol_ScrollbarGrabHovered]  = accent_color;
        colors[ImGuiCol_ScrollbarGrabActive]   = accent_color;
        colors[ImGuiCol_CheckMark]             = accent_color;
        colors[ImGuiCol_SliderGrab]            = accent_color;
        colors[ImGuiCol_SliderGrabActive]      = accent_color;
        colors[ImGuiCol_Button]                = inline_color;
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
        colors[ImGuiCol_ButtonActive]          = accent_color;
        colors[ImGuiCol_Header]                = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.35f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.65f);
        colors[ImGuiCol_HeaderActive]          = accent_color;
        colors[ImGuiCol_Separator]             = outline_color;
        colors[ImGuiCol_SeparatorHovered]      = accent_color;
        colors[ImGuiCol_SeparatorActive]       = accent_color;
        colors[ImGuiCol_Tab]                   = tab_bg_color;
        colors[ImGuiCol_TabHovered]            = inline_color;
        colors[ImGuiCol_TabActive]             = gradient_color;
    }

    void reset() {
        accent_color = ImVec4(0.0f / 255.0f, 122.0f / 255.0f, 230.0f / 255.0f, 1.0f);
        apply();
    }

    void draw_text(ImDrawList* draw_list, const char* text, const ImVec2& pos, ImU32 col) {
        draw_text_stroke(draw_list, text, pos, col);
    }

    void draw_text_stroke(ImDrawList* draw_list, const char* text, const ImVec2& pos, ImU32 col) {
        if (!text || !text[0])
            return;

        if (col == 0)
            col = ImGui::GetColorU32(text_color);

        ImU32 stroke_col = IM_COL32(0, 0, 0, (col >> 24) & 0xFF);
        draw_list->AddText(ImVec2(pos.x - 1.0f, pos.y - 1.0f), stroke_col, text);
        draw_list->AddText(ImVec2(pos.x,        pos.y - 1.0f), stroke_col, text);
        draw_list->AddText(ImVec2(pos.x + 1.0f, pos.y - 1.0f), stroke_col, text);
        draw_list->AddText(ImVec2(pos.x - 1.0f, pos.y),        stroke_col, text);
        draw_list->AddText(ImVec2(pos.x + 1.0f, pos.y),        stroke_col, text);
        draw_list->AddText(ImVec2(pos.x - 1.0f, pos.y + 1.0f), stroke_col, text);
        draw_list->AddText(ImVec2(pos.x,        pos.y + 1.0f), stroke_col, text);
        draw_list->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), stroke_col, text);
        draw_list->AddText(pos, col, text);
    }

    void draw_accent_bar(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 accent_col) {
        ImVec4 base = ImGui::ColorConvertU32ToFloat4(accent_col);
        ImU32 top = ImGui::ColorConvertFloat4ToU32(ImVec4(
            base.x * 1.30f > 1.0f ? 1.0f : base.x * 1.30f,
            base.y * 1.30f > 1.0f ? 1.0f : base.y * 1.30f,
            base.z * 1.30f > 1.0f ? 1.0f : base.z * 1.30f,
            base.w
        ));
        ImU32 bot = ImGui::ColorConvertFloat4ToU32(ImVec4(
            base.x * 0.70f,
            base.y * 0.70f,
            base.z * 0.70f,
            base.w
        ));

        draw_list->AddRectFilledMultiColor(min, max, top, top, bot, bot);
    }

    void draw_gradient_v(ImDrawList* draw_list, const ImVec2& min, const ImVec2& max, ImU32 col_top, ImU32 col_bot) {
        draw_list->AddRectFilledMultiColor(min, max, col_top, col_top, col_bot, col_bot);
    }

}

#pragma once

#include "../../imgui/imgui.h"
#include <string>
#include <vector>

namespace gui {

    enum class Tab : int {
        Legit = 0,
        Rage,
        Visuals,
        Misc,
        Settings,
        Count
    };

    namespace notifications {
        enum class Type {
            Info,
            Success,
            Warning,
            Error
        };

        struct Notification {
            std::string title;
            std::string message;
            Type type;
            float duration;
            float max_duration;
            float current_x;
            float alpha;
        };

        void push(const char* message, Type type = Type::Info, float duration = 3.5f);
        void push(const char* title, const char* message, Type type = Type::Info, float duration = 3.5f);
        void render();
    }

    inline bool menu_open = true;
    inline Tab current_tab = Tab::Legit;

    void init();
    void render();
    void render_watermark();
    void render_activity_window();

}


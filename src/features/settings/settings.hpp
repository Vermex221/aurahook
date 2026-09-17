#pragma once

#include <vector>
#include <string>

namespace features::settings {

    struct Settings {
        bool menu_accent = false;
        bool menu_bind = true;
        int menu_key = 0x2E; 
        int menu_key_mode = 2; 
        float accent_color[4] = { 0.0f / 255.0f, 122.0f / 255.0f, 230.0f / 255.0f, 1.0f }; 
        bool custom_menu_name = false;
        char menu_name[64] = "AuraHook";
        int selected_font = 0; 

        bool watermark = true;
        bool keybind_list = true;

        char config_name[32] = "Default";
        int selected_config = 0;
    };

    inline Settings cfg;
    inline std::vector<std::string> configs = { "Default" };

    void init_configs();
    void save_config(const std::string& name);
    void load_config(const std::string& name);
    void delete_config(const std::string& name);

    void render();

}

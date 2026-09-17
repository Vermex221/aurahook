#pragma once

namespace features::legit {

    struct WeaponSettings {
        bool enabled = false;
        int key = 0;
        int key_mode = 1;
        int target_part = 0;
        float fov = 5.0f;
        float smooth = 3.0f;
        bool draw_fov = false;
        float fov_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        bool trigger_enabled = false;
        int trigger_key = 0;
        int trigger_key_mode = 1;
        bool trigger_filter_enabled = false;
        int trigger_target_part = 0;
        int trigger_delay = 0;
        bool trigger_seeded_delay = false;
    };

    struct Settings {
        WeaponSettings weapons[6]; 
    };

    inline Settings cfg;
    int get_active_weapon_category();

    void render();
    void draw_fov();
    void run();
    void run_triggerbot();

}

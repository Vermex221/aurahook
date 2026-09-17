#pragma once

#ifdef Flat
#undef Flat
#endif
#ifdef Glow
#undef Glow
#endif
#ifdef White
#undef White
#endif
#ifdef Glass
#undef Glass
#endif
#ifdef Generic
#undef Generic
#endif
#ifdef Solid
#undef Solid
#endif
#ifdef Wireframe
#undef Wireframe
#endif

namespace features::visuals {

    struct EspTargetSettings {
        bool box = false;
        float box_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        bool name = false;
        float name_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        bool health = false;
        bool health_gradient = false;
        float health_gradient_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        bool health_outline = true;
        float health_outline_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        bool health_background = true;
        float health_bg_color[4] = { 0.0f, 0.0f, 0.0f, 0.4f };

        bool weapon = false;
        int weapon_display = 0; 
        float weapon_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        bool skeleton = false;
        float skeleton_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float skeleton_thickness = 1.0f;
    };

    enum class ChamMaterial : int {
        White = 0,
        Latex,
        Glow,
        Ghost,
        Flat,
        Glow2,
        Glass,
        Generic,
        Unlit,
        Solid,
        Wireframe,
        Bloom,
        Illuminate,
        Gost,
        Crystal,
        Gost2,
        Metallic,
        Flow,
        DarkMatter,
        Data,
        Chrome,
        Plastic,
        Energy,
        Hologram,
        Galaxy,
        Gold,
        Neon,
        Xray,
        Liquid,
        Pearl,
        Distortion,
        Outlines,
        Count
    };

    struct ChamsTargetSettings {
        bool enabled = false;
        int material = 0;
        float visible_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        bool occluded = false;
        float occluded_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        bool overlay = false;
        float overlay_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct Settings {
        EspTargetSettings enemy;
        EspTargetSettings team;
        EspTargetSettings local;

        ChamsTargetSettings chams_enemy;
        ChamsTargetSettings chams_team;
        ChamsTargetSettings chams_local;
        ChamsTargetSettings chams_arms;
        ChamsTargetSettings chams_weapon;

        ChamsTargetSettings chams_ragdoll_enemy;
        ChamsTargetSettings chams_ragdoll_team;
        ChamsTargetSettings chams_ragdoll_local;
    };

    inline Settings cfg;

    bool is_ffa_mode();
    void render();
    void draw_esp();

}

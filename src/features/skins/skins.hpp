#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "econ_item_system.hpp"

namespace features::skins {

    struct AppliedSkin {
        int paint_kit_id = 0;
        float wear = 0.001f;
        int seed = 1;
        bool stattrak = false;
        int stattrak_count = 1337;
        int16_t override_knife_def = 0;
    };

    struct SkinSettings {
        bool enabled = true;
        std::unordered_map<int16_t, AppliedSkin> weapon_skins;
        std::vector<int16_t> applied_order;
        int16_t equipped_knife_def = 0;
        AppliedSkin equipped_knife_skin;
        int16_t equipped_glove_def = 0;
        AppliedSkin equipped_glove_skin;
        int16_t equipped_agent_ct = 0;
        int16_t equipped_agent_t = 0;
    };

    enum class TabState : int {
        Home = 0,
        SelectCategory,
        SelectItem,
        SelectSkin,
        Preview
    };

    struct SkinUIState {
        TabState state = TabState::Home;
        ItemCategory cat = ItemCategory::Weapons;
        int16_t selected_def = 0;
        int selected_paint = 0;
        float wear = 0.001f;
        int seed = 1;
        bool stattrak = false;
        int stattrak_count = 1337;
    };

    inline SkinSettings cfg;
    inline SkinUIState ui_state{};

    void update();
    void render_ui();
    void save_stattrak();
    void load_stattrak();

}

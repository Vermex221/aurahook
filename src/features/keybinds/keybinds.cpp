#include "keybinds.hpp"
#include "../legit/legit.hpp"
#include <windows.h>
#include <unordered_map>

namespace features::keybinds {

    struct InternalBind {
        std::string name;
        bool active = false;
        bool was_down = false;
    };

    static std::unordered_map<std::string, InternalBind> bind_states;
    static std::vector<BindItem> active_cache;

    void process_bind(const std::string& name, bool master_enabled, int key, int mode, std::vector<BindItem>& list) {
        InternalBind& b = bind_states[name];
        b.name = name;

        if (!master_enabled) {
            b.active = false;
            b.was_down = false;
            return;
        }

        if (mode == 0) {
            b.active = true;
            b.was_down = false;
        } else if (key == 0) {
            b.active = true;
            b.was_down = false;
        } else {
            bool is_down = (GetAsyncKeyState(key) & 0x8000) != 0;

            if (mode == 1) { 
                b.active = is_down;
            } else if (mode == 2) { 
                if (is_down && !b.was_down) {
                    b.active = !b.active;
                }
            } else if (mode == 3) {
                b.active = !is_down;
            }
            b.was_down = is_down;
        }

        if (b.active) {
            BindItem item;
            item.name = name;
            item.key = key;
            item.mode = mode;
            item.master_switch = master_enabled;
            item.active = b.active;
            item.was_down = b.was_down;
            list.push_back(item);
        }
    }

    void init() {
        bind_states.clear();
        active_cache.clear();
    }

    void update() {
        std::vector<BindItem> list;

        int cat_idx = features::legit::get_active_weapon_category();
        const auto& active_wep = features::legit::cfg.weapons[cat_idx];

        process_bind("Aim Assist", active_wep.enabled, active_wep.key, active_wep.key_mode, list);
        process_bind("Triggerbot", active_wep.trigger_enabled, active_wep.trigger_key, active_wep.trigger_key_mode, list);

        active_cache = list;
    }

    std::vector<BindItem> get_active_binds() {
        return active_cache;
    }

    bool is_active(const std::string& name) {
        auto it = bind_states.find(name);
        if (it != bind_states.end()) {
            return it->second.active;
        }
        return false;
    }

}

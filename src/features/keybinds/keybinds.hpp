#pragma once

#include <string>
#include <vector>

namespace features::keybinds {

    struct BindItem {
        std::string name;
        int key;
        int mode;
        bool master_switch;
        bool active;
        bool was_down;
    };

    void init();
    void update();
    void process_bind(const std::string& name, bool master_enabled, int key, int mode, std::vector<BindItem>& list);
    std::vector<BindItem> get_active_binds();
    bool is_active(const std::string& name);

}

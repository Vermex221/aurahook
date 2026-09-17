#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace features::misc {

    struct SpectatorInfo {
        std::string name;
        uint64_t steam_id = 0;
    };

    struct Settings {
        bool spectator_list = false;
        bool bomb_info = false;
    };

    inline Settings cfg;

    void render();
    std::vector<SpectatorInfo> get_spectators();

}

#pragma once

#include <cstdint>

namespace materials {

    struct MaterialPair {
        uintptr_t visible = 0;
        uintptr_t occluded = 0;
    };

    bool initialize();
    uintptr_t find(int id, bool occluded);

}

#pragma once

#include "../sdk.hpp"
#include <algorithm>
#include <cmath>

namespace valve::math {

    bool world_to_screen(const vector3& world, vector2& screen, const view_matrix_t& matrix, float screen_w, float screen_h);
    void calculate_bbox(const vector3& origin, const vector3& mins, const vector3& maxs, const view_matrix_t& matrix, float screen_w, float screen_h, float& min_x, float& min_y, float& max_x, float& max_y, bool& valid);

}

#include "math.hpp"

namespace valve::math {

    bool world_to_screen(const vector3& world, vector2& screen, const view_matrix_t& matrix, float screen_w, float screen_h) {
        float w = matrix.matrix[3][0] * world.x + matrix.matrix[3][1] * world.y + matrix.matrix[3][2] * world.z + matrix.matrix[3][3];
        if (w < 0.001f) return false;

        float x = matrix.matrix[0][0] * world.x + matrix.matrix[0][1] * world.y + matrix.matrix[0][2] * world.z + matrix.matrix[0][3];
        float y = matrix.matrix[1][0] * world.x + matrix.matrix[1][1] * world.y + matrix.matrix[1][2] * world.z + matrix.matrix[1][3];

        screen.x = (screen_w / 2.0f) + (x / w) * (screen_w / 2.0f);
        screen.y = (screen_h / 2.0f) - (y / w) * (screen_h / 2.0f);
        return true;
    }

    void calculate_bbox(const vector3& origin, const vector3& mins, const vector3& maxs, const view_matrix_t& matrix, float screen_w, float screen_h, float& min_x, float& min_y, float& max_x, float& max_y, bool& valid) {
        vector3 corners[8] = {
            origin + vector3{ mins.x, mins.y, mins.z },
            origin + vector3{ mins.x, maxs.y, mins.z },
            origin + vector3{ maxs.x, mins.y, mins.z },
            origin + vector3{ maxs.x, maxs.y, mins.z },
            origin + vector3{ mins.x, mins.y, maxs.z },
            origin + vector3{ mins.x, maxs.y, maxs.z },
            origin + vector3{ maxs.x, mins.y, maxs.z },
            origin + vector3{ maxs.x, maxs.y, maxs.z }
        };

        vector2 screen_corners[8];
        valid = false;
        min_x = 10000.0f; max_x = -10000.0f;
        min_y = 10000.0f; max_y = -10000.0f;

        for (int j = 0; j < 8; ++j) {
            if (world_to_screen(corners[j], screen_corners[j], matrix, screen_w, screen_h)) {
                valid = true;
                min_x = std::min(min_x, screen_corners[j].x);
                max_x = std::max(max_x, screen_corners[j].x);
                min_y = std::min(min_y, screen_corners[j].y);
                max_y = std::max(max_y, screen_corners[j].y);
            }
        }
    }

}

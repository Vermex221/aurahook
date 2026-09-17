#pragma once

#include <cstdint>
#include <cstddef>

namespace fnv1a {

    constexpr uint32_t hash(const char* str, size_t len) noexcept {
        uint32_t val = 2166136261u;
        for (size_t i = 0; i < len; ++i) {
            val ^= static_cast<uint32_t>(str[i]);
            val *= 16777619u;
        }
        return val;
    }

    inline uint32_t runtime_hash(const char* str) noexcept {
        if (!str) return 0;
        uint32_t val = 2166136261u;
        while (*str) {
            val ^= static_cast<uint32_t>(*str++);
            val *= 16777619u;
        }
        return val;
    }

}

constexpr uint32_t operator""_hash(const char* str, size_t len) noexcept {
    return fnv1a::hash(str, len);
}

struct vector2 {
    float x{}, y{};
};

struct vector3 {
    float x{}, y{}, z{};

    vector3 operator+(const vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    vector3 operator-(const vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    vector3 operator*(float f) const { return {x * f, y * f, z * f}; }
};

struct view_matrix_t {
    float matrix[4][4];
};

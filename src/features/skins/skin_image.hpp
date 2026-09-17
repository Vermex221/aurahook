#pragma once

#include <d3d11.h>
#include <string>
#include "../../imgui/imgui.h"

namespace features::skins::skin_img {

    void init(ID3D11Device* device);
    void shutdown();
    void on_device_reset();

    ImTextureID get(const std::string& path);
    ImTextureID get_paint(const char* simple_name, const char* kit_token, int paint_kit_id);
    ImTextureID get_weapon(const char* weapon_name);

    std::string paint_path(const char* simple_name, const char* kit_token);
    std::string model_path(const char* simple_name);

}

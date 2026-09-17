#pragma once

#include <d3d11.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <cstdint>

namespace valve::icons {

    struct IconData {
        ID3D11ShaderResourceView* texture = nullptr;
        float width = 0.0f;
        float height = 0.0f;
    };

    bool initialize(ID3D11Device* device);
    void shutdown();
    IconData* get(const std::string& name, float scale = 0.35f);

}


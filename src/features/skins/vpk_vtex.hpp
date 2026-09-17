#pragma once

#include <d3d11.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <memory>

namespace features::skins::vpk_vtex {

    struct VtexImage {
        ID3D11ShaderResourceView* srv = nullptr;
        int width = 0;
        int height = 0;
    };

    struct VpkFileEntry {
        uint16_t archive_index = 0;
        uint32_t entry_offset = 0;
        uint32_t entry_length = 0;
        uint16_t preload_bytes = 0;
        uint32_t preload_pos = 0;
    };

    void init(ID3D11Device* device);
    void shutdown();
    void tick();

    bool is_vpk_loaded();
    const std::unordered_map<std::string, VpkFileEntry>& get_vpk_index();

    VtexImage* get_skin_image(const std::string& key);
    VtexImage* get_skin_image(int16_t def_index, int paint_kit_id);

}

#include "icons.hpp"
#include <windows.h>
#include <fstream>
#include <unordered_set>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

namespace valve::icons {

    static ID3D11Device* g_device = nullptr;
    static std::unordered_map<std::string, std::vector<uint8_t>> g_pending_svgs;
    static std::unordered_map<std::string, IconData> g_cached_icons;
    static bool g_initialized = false;

#pragma pack(push, 1)
    struct vpk_header {
        uint32_t signature;
        uint32_t version;
        uint32_t tree_size;
        uint32_t file_data_section_size;
        uint32_t archive_md5_section_size;
        uint32_t other_md5_section_size;
        uint32_t signature_section_size;
    };

    struct vpk_entry {
        uint32_t crc;
        uint16_t preload_bytes;
        uint16_t archive_index;
        uint32_t entry_offset;
        uint32_t entry_length;
        uint16_t terminator;
    };
#pragma pack(pop)

    static std::vector<char> decompile_vsvg(const std::vector<uint8_t>& data) {
        if (data.size() < 16) return {};
        uint16_t version = *reinterpret_cast<const uint16_t*>(data.data() + 4);
        uint32_t block_count = *reinterpret_cast<const uint32_t*>(data.data() + 12);

        if (version != 12 || block_count == 0 || block_count > 64) return {};

        constexpr uint32_t header_size = 16;
        constexpr uint32_t block_entry_size = 12;
        constexpr uint32_t data_fourcc = 'D' | ('A' << 8) | ('T' << 16) | ('A' << 24);

        for (uint32_t i = 0; i < block_count; ++i) {
            uint32_t entry_pos = header_size + i * block_entry_size;
            if (entry_pos + block_entry_size > data.size()) break;

            uint32_t type = *reinterpret_cast<const uint32_t*>(data.data() + entry_pos);
            uint32_t offset = *reinterpret_cast<const uint32_t*>(data.data() + entry_pos + 4);
            uint32_t block_size = *reinterpret_cast<const uint32_t*>(data.data() + entry_pos + 8);

            if (type != data_fourcc) continue;

            size_t data_start = static_cast<size_t>(entry_pos + 4) + offset;
            if (data_start + block_size > data.size()) break;

            const char* block_ptr = reinterpret_cast<const char*>(data.data() + data_start);
            for (size_t j = 0; j + 4 < block_size; ++j) {
                if (block_ptr[j] == '<' && block_ptr[j + 1] == 's' && block_ptr[j + 2] == 'v' && block_ptr[j + 3] == 'g') {
                    for (size_t k = block_size; k > j + 5; --k) {
                        if (block_ptr[k - 1] == '>' && block_ptr[k - 2] == 'g' && block_ptr[k - 3] == 'v' && block_ptr[k - 4] == 's') {
                            std::vector<char> xml(block_ptr + j, block_ptr + k);
                            xml.push_back('\0');
                            return xml;
                        }
                    }
                }
            }
            break;
        }
        return {};
    }

    static bool load_vpk(const std::filesystem::path& vpk_dir_path) {
        std::ifstream file(vpk_dir_path, std::ios::binary);
        if (!file.is_open()) return false;

        vpk_header header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (header.signature != 0x55AA1234 || header.version != 2) return false;

        auto tree_end = static_cast<std::streamoff>(sizeof(vpk_header)) + static_cast<std::streamoff>(header.tree_size);
        auto base_dir = vpk_dir_path.parent_path();

        static const std::unordered_set<std::string> targets = {
            "ak47", "m4a1", "m4a1_silencer", "awp", "deagle", "glock", "usp_silencer", "hkp2000",
            "p250", "cz75a", "fiveseven", "tec9", "revolver", "elite", "mac10", "mp9", "mp7", "mp5sd",
            "bizon", "p90", "ump45", "galilar", "famas", "aug", "sg556", "nova", "sawedoff", "xm1014",
            "mag7", "ssg08", "scar20", "g3sg1", "m249", "negev", "hegrenade", "flashbang", "smokegrenade",
            "molotov", "incgrenade", "decoy", "taser", "knife", "c4"
        };

        while (file.tellg() < tree_end) {
            std::string ext;
            std::getline(file, ext, '\0');
            if (ext.empty()) break;

            bool is_svg = (ext == "vsvg_c" || ext == "vsvg");

            while (true) {
                std::string dir_path;
                std::getline(file, dir_path, '\0');
                if (dir_path.empty()) break;

                bool is_equipment = is_svg && (dir_path.find("equipment") != std::string::npos);

                while (true) {
                    std::string filename;
                    std::getline(file, filename, '\0');
                    if (filename.empty()) break;

                    vpk_entry entry{};
                    file.read(reinterpret_cast<char*>(&entry), sizeof(entry));

                    if (entry.preload_bytes > 0) {
                        file.seekg(entry.preload_bytes, std::ios::cur);
                    }

                    if (!is_equipment || targets.find(filename) == targets.end()) continue;

                    char archive_name[32];
                    std::snprintf(archive_name, sizeof(archive_name), "pak01_%03d.vpk", entry.archive_index);
                    std::filesystem::path archive_path = base_dir / archive_name;

                    std::ifstream archive(archive_path, std::ios::binary);
                    if (archive.is_open()) {
                        archive.seekg(entry.entry_offset);
                        std::vector<uint8_t> raw(entry.entry_length);
                        archive.read(reinterpret_cast<char*>(raw.data()), entry.entry_length);

                        auto xml = decompile_vsvg(raw);
                        if (!xml.empty()) {
                            g_pending_svgs[filename] = std::vector<uint8_t>(xml.begin(), xml.end());
                        }
                    }
                }
            }
        }
        return !g_pending_svgs.empty();
    }

    bool initialize(ID3D11Device* device) {
        g_device = device;
        if (g_initialized) return true;

        wchar_t client_path[MAX_PATH] = { 0 };
        HMODULE h_client = GetModuleHandleW(L"client.dll");
        if (!h_client) return false;

        GetModuleFileNameW(h_client, client_path, MAX_PATH);
        std::filesystem::path p(client_path);
        std::filesystem::path csgo_dir = p.parent_path().parent_path().parent_path();
        std::filesystem::path vpk_path = csgo_dir / "pak01_dir.vpk";

        g_initialized = load_vpk(vpk_path);
        return g_initialized;
    }

    void shutdown() {
        for (auto& [name, icon] : g_cached_icons) {
            if (icon.texture) {
                icon.texture->Release();
                icon.texture = nullptr;
            }
        }
        g_cached_icons.clear();
        g_pending_svgs.clear();
        g_initialized = false;
    }

    IconData* get(const std::string& name, float scale) {
        if (!g_device) return nullptr;

        std::string key_name = name;
        if (key_name.rfind("weapon_", 0) == 0) key_name.erase(0, 7);
        if (key_name == "knife_t" || key_name == "knife_ct" || key_name.find("knife") != std::string::npos || key_name.find("bayonet") != std::string::npos) key_name = "knife";
        else if (key_name == "scout") key_name = "ssg08";
        else if (key_name == "usps" || key_name == "usp-s") key_name = "usp_silencer";
        else if (key_name == "m4a1s" || key_name == "m4a1-s") key_name = "m4a1_silencer";
        else if (key_name == "pistol") key_name = "usp_silencer";
        else if (key_name == "smg") key_name = "mp7";
        else if (key_name == "shotgun") key_name = "nova";
        else if (key_name == "rifle") key_name = "ak47";
        else if (key_name == "sniper") key_name = "ssg08";
        else if (key_name == "lmg") key_name = "negev";

        std::string cache_key = key_name + "_" + std::to_string(static_cast<int>(scale * 1000.0f));

        auto cached_it = g_cached_icons.find(cache_key);
        if (cached_it != g_cached_icons.end()) {
            return &cached_it->second;
        }

        auto pending_it = g_pending_svgs.find(key_name);
        if (pending_it == g_pending_svgs.end()) {
            return nullptr;
        }

        const auto& svg_bytes = pending_it->second;
        if (svg_bytes.empty()) return nullptr;

        NSVGimage* img = nsvgParse(reinterpret_cast<char*>(const_cast<uint8_t*>(svg_bytes.data())), "px", 96.0f);
        if (!img) return nullptr;

        int width = static_cast<int>(img->width * scale);
        int height = static_cast<int>(img->height * scale);
        if (width <= 0 || height <= 0) {
            nsvgDelete(img);
            return nullptr;
        }

        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if (!rast) {
            nsvgDelete(img);
            return nullptr;
        }

        std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4, 0);
        nsvgRasterize(rast, img, 0, 0, scale, pixels.data(), width, height, width * 4);
        nsvgDeleteRasterizer(rast);
        nsvgDelete(img);

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init_data{};
        init_data.pSysMem = pixels.data();
        init_data.SysMemPitch = width * 4;

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = g_device->CreateTexture2D(&desc, &init_data, &tex);
        if (FAILED(hr) || !tex) return nullptr;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = g_device->CreateShaderResourceView(tex, nullptr, &srv);
        tex->Release();
        if (FAILED(hr) || !srv) return nullptr;

        IconData icon{};
        icon.texture = srv;
        icon.width = static_cast<float>(width);
        icon.height = static_cast<float>(height);

        auto [it, inserted] = g_cached_icons.emplace(cache_key, icon);
        return &it->second;
    }

}


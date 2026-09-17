#include "vpk_vtex.hpp"
#include "econ_item_system.hpp"
#pragma warning(push)
#pragma warning(disable: 4996)
#include <lz4/lz4.h>
#include <lz4/lz4.c>
#pragma warning(pop)
#include <bc7.hpp>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <deque>
#include <condition_variable>
#include <unordered_set>
#include <algorithm>

namespace features::skins::vpk_vtex {

    static ID3D11Device* g_device = nullptr;

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

    enum class ImageState : uint8_t { Empty, Loading, Decoded, Ready, Failed };

    struct ImageItem {
        ImageState state = ImageState::Empty;
        VtexImage image{};
        uint16_t width = 0;
        uint16_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        std::vector<uint8_t> rgba;
    };

    static std::mutex g_mu;
    static std::unordered_map<std::string, ImageItem> g_cache;
    static std::unordered_map<std::string, VpkFileEntry> g_vpk_index;
    static std::filesystem::path g_csgo_dir;
    static uint32_t g_vpk_dir_data_offset = 0;
    static bool g_vpk_loaded = false;

    const std::unordered_map<std::string, VpkFileEntry>& get_vpk_index() {
        return g_vpk_index;
    }

    bool is_vpk_loaded() {
        return g_vpk_loaded;
    }

    static std::mutex g_queue_mu;
    static std::condition_variable g_queue_cv;
    static std::deque<std::string> g_queue;
    static std::unordered_set<std::string> g_queued_set;
    static std::atomic<bool> g_run{ false };
    static std::thread g_worker;

    static std::filesystem::path find_csgo_directory() {
        HMODULE h_client = GetModuleHandleW(L"client.dll");
        if (h_client) {
            wchar_t client_path[MAX_PATH] = { 0 };
            GetModuleFileNameW(h_client, client_path, MAX_PATH);
            std::filesystem::path p(client_path);
            auto candidate = p.parent_path().parent_path().parent_path();
            if (std::filesystem::exists(candidate / "pak01_dir.vpk"))
                return candidate;
        }

        HMODULE h_main = GetModuleHandleW(nullptr);
        if (h_main) {
            wchar_t exe_path[MAX_PATH] = { 0 };
            GetModuleFileNameW(h_main, exe_path, MAX_PATH);
            std::filesystem::path p(exe_path);
            auto candidate = p.parent_path().parent_path().parent_path() / "csgo";
            if (std::filesystem::exists(candidate / "pak01_dir.vpk"))
                return candidate;
            auto candidate2 = p.parent_path().parent_path() / "csgo";
            if (std::filesystem::exists(candidate2 / "pak01_dir.vpk"))
                return candidate2;
        }

        if (std::filesystem::exists("csgo/pak01_dir.vpk"))
            return std::filesystem::absolute("csgo");
        if (std::filesystem::exists("../csgo/pak01_dir.vpk"))
            return std::filesystem::absolute("../csgo");

        return {};
    }

    static bool index_vpk() {
        if (g_vpk_loaded) return true;
        g_csgo_dir = find_csgo_directory();
        if (g_csgo_dir.empty()) return false;

        std::filesystem::path vpk_path = g_csgo_dir / "pak01_dir.vpk";
        std::ifstream file(vpk_path, std::ios::binary);
        if (!file.is_open()) return false;

        vpk_header header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (header.signature != 0x55AA1234 || header.version != 2) return false;

        g_vpk_dir_data_offset = static_cast<uint32_t>(sizeof(vpk_header)) + header.tree_size;
        auto tree_end = static_cast<std::streamoff>(sizeof(vpk_header)) + static_cast<std::streamoff>(header.tree_size);

        while (file.tellg() < tree_end) {
            std::string ext;
            std::getline(file, ext, '\0');
            if (ext.empty()) break;

            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            bool is_vtex = (ext == "vtex_c");

            while (true) {
                std::string dir_path;
                std::getline(file, dir_path, '\0');
                if (dir_path.empty()) break;

                std::transform(dir_path.begin(), dir_path.end(), dir_path.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                bool is_econ = is_vtex && (dir_path.find("panorama/images/econ") != std::string::npos);

                while (true) {
                    std::string filename;
                    std::getline(file, filename, '\0');
                    if (filename.empty()) break;

                    vpk_entry entry{};
                    file.read(reinterpret_cast<char*>(&entry), sizeof(entry));

                    uint32_t preload_pos = static_cast<uint32_t>(file.tellg());
                    if (entry.preload_bytes > 0) {
                        file.seekg(entry.preload_bytes, std::ios::cur);
                    }

                    if (!is_econ) continue;

                    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c) { return (char)std::tolower(c); });

                    size_t p_pos = dir_path.find("panorama/images/");
                    std::string rel_dir = (p_pos != std::string::npos) ? dir_path.substr(p_pos + 16) : dir_path;

                    std::string key = rel_dir + "/" + filename;
                    g_vpk_index[key] = { entry.archive_index, entry.entry_offset, entry.entry_length, entry.preload_bytes, preload_pos };
                }
            }
        }

        g_vpk_loaded = !g_vpk_index.empty();
        return g_vpk_loaded;
    }

    static std::vector<uint8_t> read_vpk_file(const VpkFileEntry& entry) {
        std::vector<uint8_t> data;
        data.reserve(entry.preload_bytes + entry.entry_length);

        if (entry.preload_bytes > 0) {
            std::ifstream dir_file(g_csgo_dir / "pak01_dir.vpk", std::ios::binary);
            if (dir_file.is_open()) {
                dir_file.seekg(entry.preload_pos);
                size_t at = data.size();
                data.resize(at + entry.preload_bytes);
                dir_file.read(reinterpret_cast<char*>(data.data() + at), entry.preload_bytes);
            }
        }

        if (entry.entry_length > 0) {
            if (entry.archive_index == 0x7fff) {
                std::ifstream dir_file(g_csgo_dir / "pak01_dir.vpk", std::ios::binary);
                if (dir_file.is_open()) {
                    dir_file.seekg(g_vpk_dir_data_offset + entry.entry_offset);
                    size_t at = data.size();
                    data.resize(at + entry.entry_length);
                    dir_file.read(reinterpret_cast<char*>(data.data() + at), entry.entry_length);
                }
            } else {
                char archive_name[32];
                std::snprintf(archive_name, sizeof(archive_name), "pak01_%03d.vpk", entry.archive_index);
                std::filesystem::path archive_path = g_csgo_dir / archive_name;

                std::ifstream archive(archive_path, std::ios::binary);
                if (archive.is_open()) {
                    archive.seekg(entry.entry_offset);
                    size_t at = data.size();
                    data.resize(at + entry.entry_length);
                    archive.read(reinterpret_cast<char*>(data.data() + at), entry.entry_length);
                }
            }
        }

        return data;
    }

    static bool decode_vtex_buffer(const std::vector<uint8_t>& data, ImageItem& out) {
        const auto* raw = data.data();
        const auto size = data.size();

        if (size < 28) return false;

        const auto file_size = *reinterpret_cast<const uint32_t*>(raw + 0x00);
        const auto header_version = *reinterpret_cast<const uint16_t*>(raw + 0x04);
        const auto block_count = *reinterpret_cast<const uint32_t*>(raw + 0x0C);

        if (header_version != 12 || block_count == 0 || block_count > 64) return false;

        constexpr auto block_header_size = 16u;
        constexpr auto block_entry_size = 12u;
        constexpr auto data_fourcc = 'D' | ('A' << 8) | ('T' << 16) | ('A' << 24);

        const uint8_t* data_block = nullptr;
        size_t data_block_offset = 0;

        for (uint32_t i = 0; i < block_count; i++) {
            const auto entry_pos = block_header_size + i * block_entry_size;
            if (entry_pos + block_entry_size > size) break;

            const auto type = *reinterpret_cast<const uint32_t*>(raw + entry_pos);
            const auto offset = *reinterpret_cast<const uint32_t*>(raw + entry_pos + 4);

            if (type != data_fourcc) continue;

            const auto data_start = entry_pos + 4 + offset;
            if (data_start + 0x28 > size) return false;

            data_block = raw + data_start;
            data_block_offset = data_start;
            break;
        }

        if (!data_block) return false;

        const auto width = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(data_block + 0x14));
        const auto height = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(data_block + 0x16));
        const auto format = *reinterpret_cast<const uint8_t*>(data_block + 0x1A);
        const auto mip_count = static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(data_block + 0x1B));
        const auto extra_data_offset = *reinterpret_cast<const uint32_t*>(data_block + 0x20);
        const auto extra_data_count = *reinterpret_cast<const uint32_t*>(data_block + 0x24);

        if (width == 0 || height == 0 || mip_count == 0) return false;

        uint32_t block_bytes = 0;
        uint32_t bytes_per_pixel = 0;

        switch (format) {
        case 1: block_bytes = 8; break;
        case 2: block_bytes = 16; break;
        case 4: bytes_per_pixel = 4; break;
        case 20: block_bytes = 16; break;
        case 28: bytes_per_pixel = 4; break;
        default: return false;
        }

        auto calc_mip_size = [&](uint32_t w, uint32_t h) -> uint32_t {
            if (block_bytes > 0) {
                const auto bw = (std::max)(4u, (w + 3u) & ~3u);
                const auto bh = (std::max)(4u, (h + 3u) & ~3u);
                return (bw / 4) * (bh / 4) * block_bytes;
            }
            return w * h * bytes_per_pixel;
        };

        constexpr auto extra_compressed_mip_size = 4u;
        bool is_compressed = false;
        const uint32_t* compressed_sizes = nullptr;
        uint32_t compressed_sizes_count = 0;

        if (extra_data_count > 0) {
            const auto table_pos = 0x20 + extra_data_offset;
            for (uint32_t i = 0; i < extra_data_count; i++) {
                const auto entry_pos = table_pos + i * 12;
                if (data_block_offset + entry_pos + 12 > size) return false;

                const auto etype = *reinterpret_cast<const uint32_t*>(data_block + entry_pos);
                const auto eoff = *reinterpret_cast<const uint32_t*>(data_block + entry_pos + 4);
                const auto esize = *reinterpret_cast<const uint32_t*>(data_block + entry_pos + 8);

                if (etype != extra_compressed_mip_size) continue;

                const auto body_pos = entry_pos + 4 + eoff;
                if (data_block_offset + body_pos + 12 > size || esize < 12) return false;

                const auto int1 = *reinterpret_cast<const uint32_t*>(data_block + body_pos);
                const auto mips_offset = *reinterpret_cast<const uint32_t*>(data_block + body_pos + 4);
                const auto mips_count_in_table = *reinterpret_cast<const uint32_t*>(data_block + body_pos + 8);

                if (int1 > 1 || mips_count_in_table != mip_count) return false;

                const auto array_pos = body_pos + 4 + mips_offset;
                if (data_block_offset + array_pos + mips_count_in_table * 4u > size) return false;

                is_compressed = (int1 == 1);
                compressed_sizes = reinterpret_cast<const uint32_t*>(data_block + array_pos);
                compressed_sizes_count = mips_count_in_table;
                break;
            }
        }

        const auto pixel_start = static_cast<size_t>(file_size);
        if (pixel_start >= size) return false;

        auto on_disk_size_for = [&](uint32_t mip_level) -> uint32_t {
            const auto mw = (std::max)(1u, width >> mip_level);
            const auto mh = (std::max)(1u, height >> mip_level);
            const auto uncompressed = calc_mip_size(mw, mh);

            if (!is_compressed || compressed_sizes == nullptr || mip_level >= compressed_sizes_count)
                return uncompressed;

            const auto compressed = compressed_sizes[mip_level];
            return (compressed >= uncompressed) ? uncompressed : compressed;
        };

        std::vector<uint8_t> mip0_data;
        auto cursor = pixel_start;

        for (int j = (int)mip_count - 1; j >= 0; --j) {
            const auto on_disk = on_disk_size_for((uint32_t)j);
            if (cursor + on_disk > size) return false;

            if (j == 0) {
                const auto uncompressed = calc_mip_size(width, height);
                mip0_data.resize(uncompressed);

                if (!is_compressed || on_disk >= uncompressed) {
                    if (on_disk != uncompressed) return false;
                    std::memcpy(mip0_data.data(), raw + cursor, uncompressed);
                } else {
                    const auto decoded = LZ4_decompress_safe(reinterpret_cast<const char*>(raw + cursor), reinterpret_cast<char*>(mip0_data.data()), (int)on_disk, (int)uncompressed);
                    if (decoded != (int)uncompressed) return false;
                }
            }
            cursor += on_disk;
        }

        out.width = (uint16_t)width;
        out.height = (uint16_t)height;
        out.format = DXGI_FORMAT_R8G8B8A8_UNORM;

        if (format == 20) { 
            out.rgba.resize((size_t)width * height * 4);
            bc7::decode_image(mip0_data.data(), out.rgba.data(), (int)width, (int)height);
        } else if (format == 4) { 
            out.rgba = std::move(mip0_data);
        } else if (format == 28) { 
            out.rgba = std::move(mip0_data);
            for (size_t i = 0; i < out.rgba.size(); i += 4) {
                std::swap(out.rgba[i], out.rgba[i + 2]);
            }
        } else {
            return false;
        }

        return true;
    }

    static std::mutex g_decoded_mu;
    static std::vector<std::string> g_decoded_keys;

    static void worker_fn() {
        while (g_run.load()) {
            std::string key;
            {
                std::unique_lock<std::mutex> lock(g_queue_mu);
                g_queue_cv.wait_for(lock, std::chrono::milliseconds(200), [] {
                    return !g_queue.empty() || !g_run.load();
                });
                if (!g_run.load()) break;
                if (g_queue.empty()) continue;
                key = std::move(g_queue.front());
                g_queue.pop_front();
            }

            {
                std::lock_guard<std::mutex> ql(g_queue_mu);
                g_queued_set.erase(key);
            }

            VpkFileEntry entry{};
            bool found = false;
            {
                std::lock_guard<std::mutex> lock(g_mu);
                auto it = g_vpk_index.find(key);
                if (it != g_vpk_index.end()) {
                    entry = it->second;
                    found = true;
                }
            }

            if (!found) {
                std::lock_guard<std::mutex> lock(g_mu);
                g_cache[key].state = ImageState::Failed;
                continue;
            }

            auto raw = read_vpk_file(entry);
            if (raw.empty()) {
                std::lock_guard<std::mutex> lock(g_mu);
                g_cache[key].state = ImageState::Failed;
                continue;
            }

            ImageItem decoded{};
            if (decode_vtex_buffer(raw, decoded)) {
                {
                    std::lock_guard<std::mutex> lock(g_mu);
                    auto& item = g_cache[key];
                    item.width = decoded.width;
                    item.height = decoded.height;
                    item.format = decoded.format;
                    item.rgba = std::move(decoded.rgba);
                    item.state = ImageState::Decoded;
                }
                {
                    std::lock_guard<std::mutex> dl(g_decoded_mu);
                    g_decoded_keys.push_back(key);
                }
            } else {
                std::lock_guard<std::mutex> lock(g_mu);
                g_cache[key].state = ImageState::Failed;
            }
        }
    }

    void init(ID3D11Device* device) {
        g_device = device;
        if (!g_run.load()) {
            index_vpk();
            g_run.store(true);
            g_worker = std::thread(worker_fn);
        }
    }

    void shutdown() {
        g_run.store(false);
        g_queue_cv.notify_all();
        if (g_worker.joinable()) g_worker.join();

        std::lock_guard<std::mutex> lock(g_mu);
        for (auto& [k, item] : g_cache) {
            if (item.image.srv) {
                item.image.srv->Release();
                item.image.srv = nullptr;
            }
        }
        g_cache.clear();
        g_vpk_index.clear();
        g_vpk_loaded = false;
        g_device = nullptr;
    }

    void tick() {
        if (!g_device) return;

        std::vector<std::string> to_upload;
        {
            std::lock_guard<std::mutex> dl(g_decoded_mu);
            if (g_decoded_keys.empty()) return;
            size_t count = (std::min)(g_decoded_keys.size(), static_cast<size_t>(16));
            to_upload.assign(g_decoded_keys.begin(), g_decoded_keys.begin() + count);
            g_decoded_keys.erase(g_decoded_keys.begin(), g_decoded_keys.begin() + count);
        }

        std::lock_guard<std::mutex> lock(g_mu);
        for (const auto& key : to_upload) {
            auto it = g_cache.find(key);
            if (it == g_cache.end()) continue;
            auto& item = it->second;
            if (item.state != ImageState::Decoded || item.rgba.empty()) continue;

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = item.width;
            desc.Height = item.height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = item.format;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA init_data{};
            init_data.pSysMem = item.rgba.data();
            init_data.SysMemPitch = item.width * 4;

            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(g_device->CreateTexture2D(&desc, &init_data, &tex)) && tex) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format = desc.Format;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv_desc.Texture2D.MipLevels = 1;

                if (SUCCEEDED(g_device->CreateShaderResourceView(tex, &srv_desc, &item.image.srv))) {
                    item.image.width = item.width;
                    item.image.height = item.height;
                    item.state = ImageState::Ready;
                } else {
                    item.state = ImageState::Failed;
                }
                tex->Release();
            } else {
                item.state = ImageState::Failed;
            }
            item.rgba.clear();
            item.rgba.shrink_to_fit();
        }
    }

    VtexImage* get_skin_image(const std::string& key) {
        if (!g_device) return nullptr;

        std::lock_guard<std::mutex> lock(g_mu);
        auto it = g_cache.find(key);
        if (it != g_cache.end()) {
            if (it->second.state == ImageState::Ready) return &it->second.image;
            if (it->second.state == ImageState::Failed) return nullptr;
            return nullptr;
        }

        g_cache[key].state = ImageState::Loading;
        {
            std::lock_guard<std::mutex> ql(g_queue_mu);
            if (g_queued_set.insert(key).second) {
                g_queue.push_back(key);
            }
        }
        g_queue_cv.notify_one();
        return nullptr;
    }

    static std::string glove_default_kit_name(const std::string& simple) {
        if (simple == "sporty_gloves") return "sporty_black_webbing_yellow";
        if (simple == "slick_gloves") return "slick_snakeskin_white";
        if (simple == "leather_handwraps") return "handwrap_camo_grey";
        if (simple == "motorcycle_gloves") return "motorcycle_basic_black";
        if (simple == "specialist_gloves") return "specialist_ddpat_green_camo";
        if (simple == "studded_bloodhound_gloves") return "bloodhound_black_silver";
        if (simple == "studded_hydra_gloves") return "bloodhound_hydra_black_green";
        if (simple == "studded_brokenfang_gloves") return "operation10_floral";
        return "bloodhound_black_silver";
    }

    static std::vector<std::string> get_weapon_aliases(const std::string& raw_name) {
        std::string w = raw_name;
        if (w.rfind("weapon_", 0) == 0) w.erase(0, 7);

        std::vector<std::string> aliases;
        aliases.push_back(w);
        aliases.push_back("weapon_" + w);

        if (w == "m4a1_silencer") {
            aliases.push_back("m4a1_s");
            aliases.push_back("weapon_m4a1_s");
            aliases.push_back("m4a1s");
            aliases.push_back("weapon_m4a1s");
            aliases.push_back("m4a1");
        } else if (w == "usp_silencer") {
            aliases.push_back("usp_s");
            aliases.push_back("weapon_usp_s");
            aliases.push_back("usp");
        } else if (w == "deagle") {
            aliases.push_back("deag");
            aliases.push_back("weapon_deag");
        } else if (w == "hkp2000") {
            aliases.push_back("p2000");
            aliases.push_back("weapon_p2000");
        } else if (w == "galilar") {
            aliases.push_back("galil");
            aliases.push_back("weapon_galil");
        } else if (w == "mp5sd") {
            aliases.push_back("mp5_sd");
            aliases.push_back("weapon_mp5_sd");
        } else if (w == "sg556") {
            aliases.push_back("sg553");
            aliases.push_back("weapon_sg553");
        } else if (w == "knife_css") {
            aliases.push_back("knife_classic");
            aliases.push_back("weapon_knife_classic");
            aliases.push_back("classic");
            aliases.push_back("css");
        } else if (w == "knife_survival_bowie") {
            aliases.push_back("knife_bowie");
            aliases.push_back("weapon_knife_bowie");
            aliases.push_back("survival_bowie");
            aliases.push_back("bowie");
        } else if (w == "knife_gypsy_jackknife") {
            aliases.push_back("knife_navaja");
            aliases.push_back("weapon_knife_navaja");
            aliases.push_back("knife_gypsy");
            aliases.push_back("weapon_knife_gypsy");
            aliases.push_back("navaja");
            aliases.push_back("gypsy");
        } else if (w == "knife_widowmaker") {
            aliases.push_back("knife_talon");
            aliases.push_back("weapon_knife_talon");
            aliases.push_back("knife_widowmaker");
            aliases.push_back("talon");
            aliases.push_back("widowmaker");
        } else if (w == "knife_outdoor") {
            aliases.push_back("knife_nomad");
            aliases.push_back("weapon_knife_nomad");
            aliases.push_back("knife_outdoor");
            aliases.push_back("nomad");
            aliases.push_back("outdoor");
        } else if (w == "knife_canis") {
            aliases.push_back("knife_survival");
            aliases.push_back("weapon_knife_survival");
            aliases.push_back("knife_canis");
            aliases.push_back("survival");
            aliases.push_back("canis");
        } else if (w == "knife_cord") {
            aliases.push_back("knife_paracord");
            aliases.push_back("weapon_knife_paracord");
            aliases.push_back("knife_cord");
            aliases.push_back("paracord");
            aliases.push_back("cord");
        } else if (w == "knife_push") {
            aliases.push_back("knife_shadow_daggers");
            aliases.push_back("weapon_knife_push");
            aliases.push_back("shadow_daggers");
            aliases.push_back("push");
        } else if (w == "knife_tactical") {
            aliases.push_back("knife_huntsman");
            aliases.push_back("weapon_knife_tactical");
            aliases.push_back("huntsman");
            aliases.push_back("tactical");
        } else if (w == "knife_falchion") {
            aliases.push_back("knife_falchion_advanced");
            aliases.push_back("weapon_knife_falchion_advanced");
            aliases.push_back("falchion_advanced");
            aliases.push_back("falchion");
        } else if (w == "knife_karambit") {
            aliases.push_back("knife_karam");
            aliases.push_back("weapon_knife_karam");
            aliases.push_back("karam");
            aliases.push_back("karambit");
        } else if (w == "knife_m9_bayonet") {
            aliases.push_back("knife_m9_bay");
            aliases.push_back("weapon_knife_m9_bay");
            aliases.push_back("m9_bay");
            aliases.push_back("m9_bayonet");
        } else if (w == "m4a1") {
            aliases.push_back("m4a4");
            aliases.push_back("weapon_m4a4");
        } else if (w == "elite") {
            aliases.push_back("dual_berettas");
            aliases.push_back("weapon_dual_berettas");
            aliases.push_back("dualberettas");
        } else if (w == "cz75a") {
            aliases.push_back("cz75");
            aliases.push_back("weapon_cz75");
            aliases.push_back("cz75_auto");
        } else if (w == "revolver") {
            aliases.push_back("r8");
            aliases.push_back("weapon_r8");
            aliases.push_back("revolver_r8");
        } else if (w == "bizon") {
            aliases.push_back("mp_bizon");
            aliases.push_back("weapon_mp_bizon");
        } else if (w == "ssg08") {
            aliases.push_back("scout");
            aliases.push_back("weapon_scout");
        } else if (w == "knife_butterfly") {
            aliases.push_back("butterfly");
            aliases.push_back("knife_butterfly_advanced");
        } else if (w == "knife_skeleton") {
            aliases.push_back("skeleton");
            aliases.push_back("weapon_knife_skeleton");
        } else if (w == "knife_kukri") {
            aliases.push_back("kukri");
            aliases.push_back("weapon_knife_kukri");
        } else if (w == "bayonet") {
            aliases.push_back("knife_bayonet");
            aliases.push_back("weapon_knife_bayonet");
            aliases.push_back("bayonet");
        }
        return aliases;
    }

    static std::vector<std::string> get_paint_tokens(const std::string& pk_name, const std::string& display_name = "") {
        std::vector<std::string> tokens;
        tokens.push_back(pk_name);

        auto u = pk_name.find('_');
        if (u != std::string::npos && u <= 3) {
            std::string s1 = pk_name.substr(u + 1);
            tokens.push_back(s1);
            auto u2 = s1.find('_');
            if (u2 != std::string::npos) {
                tokens.push_back(s1.substr(u2 + 1));
            }
        }

        if (!display_name.empty()) {
            std::string disp_clean;
            for (char c : display_name) {
                if (isalnum((unsigned char)c)) disp_clean += (char)tolower((unsigned char)c);
                else if (c == ' ' || c == '-' || c == '_') disp_clean += '_';
            }
            while (!disp_clean.empty() && disp_clean.back() == '_') disp_clean.pop_back();
            if (!disp_clean.empty()) {
                tokens.push_back(disp_clean);
                std::stringstream ss(disp_clean);
                std::string word;
                while (std::getline(ss, word, '_')) {
                    if (word.size() >= 4) tokens.push_back(word);
                }
            }
        }

        if (pk_name.find("ruby") != std::string::npos) {
            tokens.push_back("am_ruby_marbleized");
            tokens.push_back("ruby");
            tokens.push_back("marbleized");
        }
        if (pk_name.find("sapphire") != std::string::npos) {
            tokens.push_back("am_sapphire_marbleized");
            tokens.push_back("sapphire");
            tokens.push_back("marbleized");
        }
        if (pk_name.find("blackpearl") != std::string::npos || pk_name.find("black_pearl") != std::string::npos) {
            tokens.push_back("am_blackpearl_marbleized");
            tokens.push_back("blackpearl");
            tokens.push_back("marbleized");
        }
        if (pk_name.find("emerald") != std::string::npos) {
            tokens.push_back("am_emerald_marbleized");
            tokens.push_back("emerald");
            tokens.push_back("marbleized");
        }
        if (pk_name.find("fade") != std::string::npos) {
            tokens.push_back("aa_fade");
            tokens.push_back("fade");
        }
        if (pk_name.find("printstream") != std::string::npos) {
            tokens.push_back("printstream");
            tokens.push_back("m4a1s_printstream");
            tokens.push_back("deag_printstream");
        }
        if (pk_name.find("cyrex") != std::string::npos) {
            tokens.push_back("cyrex");
            tokens.push_back("m4a1s_cyrex");
        }
        if (pk_name.find("csgo2048") != std::string::npos || pk_name.find("player_two") != std::string::npos || pk_name.find("playertwo") != std::string::npos) {
            tokens.push_back("csgo2048");
            tokens.push_back("m4a1s_csgo2048");
        }
        if (pk_name.find("lore") != std::string::npos) {
            tokens.push_back("lore");
            tokens.push_back("bayonet_lore");
            tokens.push_back("m9_bay_lore");
        }
        if (pk_name.find("autotronic") != std::string::npos) {
            tokens.push_back("autotronic");
            tokens.push_back("bayonet_autotronic");
            tokens.push_back("m9_bay_autotronic");
        }
        return tokens;
    }

    VtexImage* get_skin_image(int16_t def_index, int paint_kit_id) {
        const auto* item = EconItemSystem::get().find_item(def_index);
        if (!item) return nullptr;

        static std::unordered_map<uint32_t, std::string> s_resolved_keys;
        uint32_t lookup_id = (static_cast<uint32_t>(def_index) << 16) | (static_cast<uint32_t>(paint_kit_id) & 0xFFFF);

        std::string chosen_key;
        {
            std::lock_guard<std::mutex> lock(g_mu);
            auto it = s_resolved_keys.find(lookup_id);
            if (it != s_resolved_keys.end()) {
                if (it->second == "__not_found__") return nullptr;
                chosen_key = it->second;
            }
        }

        if (chosen_key.empty()) {
            std::vector<std::string> weapon_aliases = get_weapon_aliases(item->name);
            std::vector<std::string> candidates;

            if (item->category == ItemCategory::Agents) {
                candidates.push_back("econ/characters/" + item->name + "_png");
                candidates.push_back("econ/characters/" + item->name);
                std::string short_name = item->name;
                if (short_name.rfind("customplayer_", 0) == 0) short_name.erase(0, 13);
                candidates.push_back("econ/characters/" + short_name + "_png");
            } else if (item->category == ItemCategory::Gloves && paint_kit_id <= 0) {
                std::string gkit = glove_default_kit_name(item->name);
                candidates.push_back("econ/default_generated/" + item->name + "_" + gkit + "_light_png");
                candidates.push_back("econ/weapons/base_weapons/" + item->name + "_png");
            } else if (paint_kit_id > 0) {
                const auto* pk = EconItemSystem::get().find_paint_kit(paint_kit_id);
                if (pk) {
                    std::vector<std::string> p_tokens = get_paint_tokens(pk->name, pk->display_name);
                    for (const auto& w : weapon_aliases) {
                        for (const auto& p : p_tokens) {
                            candidates.push_back("econ/default_generated/weapon_" + w + "_" + p + "_light_png");
                            candidates.push_back("econ/default_generated/" + w + "_" + p + "_light_png");
                            candidates.push_back("econ/default_generated/weapon_" + w + "_" + p + "_medium_png");
                            candidates.push_back("econ/default_generated/weapon_" + w + "_" + p + "_png");
                        }
                    }
                }
            } else {
                for (const auto& w : weapon_aliases) {
                    candidates.push_back("econ/weapons/base_weapons/weapon_" + w + "_png");
                    candidates.push_back("econ/weapons/base_weapons/" + w + "_png");
                    candidates.push_back("econ/default_generated/weapon_" + w + "_light_png");
                    candidates.push_back("econ/default_generated/" + w + "_light_png");
                }
            }

            {
                std::lock_guard<std::mutex> lock(g_mu);
                for (const auto& c : candidates) {
                    if (g_vpk_index.find(c) != g_vpk_index.end()) {
                        chosen_key = c;
                        break;
                    }
                }

                if (chosen_key.empty() && paint_kit_id > 0) {
                    const auto* pk = EconItemSystem::get().find_paint_kit(paint_kit_id);
                    if (pk) {
                        std::vector<std::string> p_tokens = get_paint_tokens(pk->name, pk->display_name);
                        for (const auto& [vpk_path, _] : g_vpk_index) {
                            if (vpk_path.rfind("econ/default_generated/", 0) == 0 && vpk_path.ends_with("_light_png")) {
                                bool has_w = false;
                                for (const auto& w : weapon_aliases) {
                                    if (vpk_path.find(w) != std::string::npos) { has_w = true; break; }
                                }
                                if (has_w) {
                                    bool has_p = false;
                                    for (const auto& p : p_tokens) {
                                        if (vpk_path.find(p) != std::string::npos) { has_p = true; break; }
                                    }
                                    if (has_p) {
                                        chosen_key = vpk_path;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                
                if (chosen_key.empty() && paint_kit_id <= 0 && item->category != ItemCategory::Agents) {
                    for (const auto& [vpk_path, _] : g_vpk_index) {
                        if (vpk_path.rfind("econ/weapons/base_weapons/", 0) == 0) {
                            for (const auto& w : weapon_aliases) {
                                if (vpk_path.find(w) != std::string::npos) {
                                    chosen_key = vpk_path;
                                    break;
                                }
                            }
                            if (!chosen_key.empty()) break;
                        }
                    }
                }

                
                if (chosen_key.empty() && item->category == ItemCategory::Agents) {
                    for (const auto& [vpk_path, _] : g_vpk_index) {
                        if (vpk_path.rfind("econ/characters/", 0) == 0 && vpk_path.find(item->name.substr(0, (std::min)((size_t)25, item->name.size()))) != std::string::npos) {
                            chosen_key = vpk_path;
                            break;
                        }
                    }
                }

                if (g_vpk_loaded) {
                    s_resolved_keys[lookup_id] = chosen_key.empty() ? "__not_found__" : chosen_key;
                }
            }
        }

        if (chosen_key.empty() || chosen_key == "__not_found__") {
            return nullptr;
        }

        return get_skin_image(chosen_key);
    }

}

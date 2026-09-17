#include "materials.hpp"
#include "vmats.hpp"
#include "../../../core/memory/memory.hpp"
#include <array>
#include <vector>

namespace materials {

    struct key_values3 {
        uint64_t metadata;
        uint64_t payload;
    };

    struct kv3_id {
        const char* format;
        uintptr_t guid_low;
        uintptr_t guid_high;
    };

    struct strong_handle {
        const void* binding;
    };

    static std::array<MaterialPair, 33> loaded_materials{};
    static std::vector<strong_handle> material_handles{};
    static bool is_initialized = false;

    static uintptr_t load_material(const char* vmat_data, const char* name) {
        if (!vmat_data || !name) return 0;

        constexpr kv3_id id{ "generic", 0x41B818518343427E, 0xB5F447C23C0CDF8C };

        static uintptr_t kv3_set_type = memory::pattern_scan("tier0.dll", "40 53 48 83 EC 30 80 FA 06 0F B6 C2 41 B9 16");
        static uintptr_t kv3_destroy = memory::pattern_scan("tier0.dll", "40 57 41 57 48 83 EC 38 4C 8B 01 44 8B FA 49 8B C0 48 8B F9 48 C1 E8 02");
        static auto kv3_load = reinterpret_cast<bool(*)(void*, void*, const char*, const kv3_id*, void*, unsigned int)>(
            GetProcAddress(GetModuleHandleA("tier0.dll"), "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z")
        );
        static uintptr_t material_create = memory::pattern_scan("materialsystem2.dll", "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 8B F2");

        if (!kv3_set_type || !kv3_destroy || !kv3_load || !material_create) {
            return 0;
        }

        key_values3 kv3{};
        typedef key_values3*(*kv3_alloc_fn)(key_values3*, unsigned int, unsigned int);
        if (reinterpret_cast<kv3_alloc_fn>(kv3_set_type)(&kv3, 1u, 6u) != &kv3) return 0;

        strong_handle handle{};
        bool loaded = kv3_load(&kv3, nullptr, vmat_data, &id, nullptr, 0u);
        if (loaded) {
            typedef void*(*material_create_fn)(void*, strong_handle*, const char*, key_values3*, int, bool);
            reinterpret_cast<material_create_fn>(material_create)(nullptr, &handle, name, &kv3, 0, true);
        }

        typedef void(*kv3_destroy_fn)(key_values3*, unsigned int);
        reinterpret_cast<kv3_destroy_fn>(kv3_destroy)(&kv3, 0u);

        if (!loaded || !handle.binding) return 0;

        material_handles.push_back(handle);
        return *reinterpret_cast<const uintptr_t*>(handle.binding);
    }

    bool initialize() {
        if (is_initialized) return true;

        auto load_pair = [](int idx, const char* vis_data, const char* vis_name, const char* occ_data, const char* occ_name) {
            loaded_materials[idx].visible = load_material(vis_data, vis_name);
            loaded_materials[idx].occluded = load_material(occ_data, occ_name);
            return loaded_materials[idx].visible != 0;
        };

        using namespace materials::vmat;

        load_pair(0, white_vmat, "materials/dev/white.vmat", white_vmat_invis, "materials/dev/white_invis.vmat");
        load_pair(1, latex_vmat, "materials/dev/latex.vmat", latex_vmat_invis, "materials/dev/latex_invis.vmat");
        load_pair(2, glow_vmat, "materials/dev/glow.vmat", glow_vmat_invis, "materials/dev/glow_invis.vmat");
        load_pair(3, ghost_vmat, "materials/dev/ghost.vmat", ghost_vmat_invis, "materials/dev/ghost_invis.vmat");
        load_pair(4, flat_vmat, "materials/dev/flat.vmat", flat_vmat_invis, "materials/dev/flat_invis.vmat");
        load_pair(5, bloom2_vmat, "materials/dev/glow2.vmat", bloom2_vmat_invis, "materials/dev/glow2_invis.vmat");
        load_pair(6, glass_vmat, "materials/dev/glass.vmat", glass_vmat_invis, "materials/dev/glass_invis.vmat");
        load_pair(7, generic_vmat, "materials/dev/generic.vmat", generic_vmat_invis, "materials/dev/generic_invis.vmat");
        load_pair(8, unlit_vmat, "materials/dev/unlit.vmat", unlit_vmat_invis, "materials/dev/unlit_invis.vmat");
        load_pair(9, solid_vmat, "materials/dev/solid.vmat", solid_vmat_invis, "materials/dev/solid_invis.vmat");
        load_pair(10, wireframe_vmat, "materials/dev/wireframe.vmat", wireframe_vmat_invis, "materials/dev/wireframe_invis.vmat");
        load_pair(11, bloom_vmat, "materials/dev/bloom.vmat", bloom_vmat_invis, "materials/dev/bloom_invis.vmat");
        load_pair(12, illuminate_vmat, "materials/dev/illuminate.vmat", illuminate_vmat_invis, "materials/dev/illuminate_invis.vmat");
        load_pair(13, gost_vmat, "materials/dev/gost.vmat", gost_vmat_invis, "materials/dev/gost_invis.vmat");
        load_pair(14, crystal_vmat, "materials/dev/crystal.vmat", crystal_vmat_invis, "materials/dev/crystal_invis.vmat");
        load_pair(15, gost2_vmat, "materials/dev/gost2.vmat", gost2_vmat_invis, "materials/dev/gost2_invis.vmat");
        load_pair(16, metallic_vmat, "materials/dev/metallic.vmat", metallic_vmat_invis, "materials/dev/metallic_invis.vmat");
        load_pair(17, flow_vmat, "materials/dev/flow.vmat", flow_vmat_invis, "materials/dev/flow_invis.vmat");
        load_pair(18, darkmatter_vmat, "materials/dev/darkmatter.vmat", darkmatter_vmat_invis, "materials/dev/darkmatter_invis.vmat");
        load_pair(19, data_vmat, "materials/dev/data.vmat", data_vmat_invis, "materials/dev/data_invis.vmat");
        load_pair(20, chrome_vmat, "materials/dev/chrome.vmat", chrome_vmat_invis, "materials/dev/chrome_invis.vmat");
        load_pair(21, plastic_vmat, "materials/dev/plastic.vmat", plastic_vmat_invis, "materials/dev/plastic_invis.vmat");
        load_pair(22, energy_vmat, "materials/dev/energy.vmat", energy_vmat_invis, "materials/dev/energy_invis.vmat");
        load_pair(23, hologram_vmat, "materials/dev/hologram.vmat", hologram_vmat_invis, "materials/dev/hologram_invis.vmat");
        load_pair(24, galaxy_vmat, "materials/dev/galaxy.vmat", galaxy_vmat_invis, "materials/dev/galaxy_invis.vmat");
        load_pair(25, gold_vmat, "materials/dev/gold.vmat", gold_vmat_invis, "materials/dev/gold_invis.vmat");
        load_pair(26, neon_vmat, "materials/dev/neon.vmat", neon_vmat_invis, "materials/dev/neon_invis.vmat");
        load_pair(27, xray_vmat, "materials/dev/xray.vmat", xray_vmat_invis, "materials/dev/xray_invis.vmat");
        load_pair(28, liquid_vmat, "materials/dev/liquid.vmat", liquid_vmat_invis, "materials/dev/liquid_invis.vmat");
        load_pair(29, pearl_vmat, "materials/dev/pearl.vmat", pearl_vmat_invis, "materials/dev/pearl_invis.vmat");
        load_pair(30, distortion_vmat, "materials/dev/distortion.vmat", distortion_vmat_invis, "materials/dev/distortion_invis.vmat");
        load_pair(31, outlines_vmat, "materials/dev/outlines.vmat", outlines_vmat_invis, "materials/dev/outlines_invis.vmat");

        is_initialized = true;
        return true;
    }

    uintptr_t find(int id, bool occluded) {
        if (id < 0 || id >= static_cast<int>(loaded_materials.size())) return 0;
        return occluded ? loaded_materials[id].occluded : loaded_materials[id].visible;
    }

}

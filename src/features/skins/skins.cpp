#include "skins.hpp"
#include "skin_image.hpp"
#include "vpk_vtex.hpp"
#include "../../widgets/custom_widgets.hpp"
#include "../../gui/gui/gui.hpp"
#include "../../gui/theme/theme.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"
#include "../../valve/schema/schema.hpp"
#include "../../valve/entity/entity.hpp"
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_internal.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <windows.h>

namespace features::skins {

    static int last_match_kills = -1;

    struct AppliedGunState {
        int paint_kit = 0;
        int seed = 1;
        float wear = 0.0f;
        int stattrak = -1;
        int16_t def = 0;
    };
    static std::unordered_map<uint32_t, AppliedGunState> s_applied_handles;
    static std::unordered_set<uint32_t> s_hud_synced_handles;
    static uint32_t s_last_active_weapon = 0;
    static uintptr_t s_last_pawn = 0;
    static std::atomic<uint32_t> s_item_serial{ 0x100 };

    static char s_item_search[64] = "";
    static char s_skin_search[64] = "";

    static void stattrak_path(char* buf, size_t sz) {
        snprintf(buf, sz, "configs\\stattrak.dat");
    }

    static uint32_t murmurhash2_lower(const void* key, int len, uint32_t seed) {
        const uint32_t m = 0x5bd1e995;
        const int r = 24;
        uint32_t h = seed ^ len;
        const auto* data = reinterpret_cast<const unsigned char*>(key);
        while (len >= 4) {
            uint32_t k = (static_cast<uint32_t>(tolower(data[0])))
                | (static_cast<uint32_t>(tolower(data[1])) << 8)
                | (static_cast<uint32_t>(tolower(data[2])) << 16)
                | (static_cast<uint32_t>(tolower(data[3])) << 24);
            k *= m;
            k ^= k >> r;
            k *= m;
            h *= m;
            h ^= k;
            data += 4;
            len -= 4;
        }
        switch (len) {
        case 3: h ^= static_cast<uint32_t>(tolower(data[2])) << 16; [[fallthrough]];
        case 2: h ^= static_cast<uint32_t>(tolower(data[1])) << 8;  [[fallthrough]];
        case 1: h ^= static_cast<uint32_t>(tolower(data[0]));
            h *= m;
        };
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;
        return h;
    }

    static uint32_t make_subclass_token(int16_t def_index) {
        std::string s = std::to_string(def_index);
        return murmurhash2_lower(s.c_str(), static_cast<int>(s.length()), 0x31415926);
    }

    void save_stattrak() {
        char path[256];
        stattrak_path(path, sizeof(path));
        CreateDirectoryA("configs", nullptr);
        std::ofstream f(path);
        if (!f.is_open()) return;

        f << "knife=" << cfg.equipped_knife_skin.stattrak_count << "\n";
        f << "glove=" << cfg.equipped_glove_skin.stattrak_count << "\n";
        for (const auto& [def, skin] : cfg.weapon_skins) {
            if (skin.stattrak)
                f << "w" << (int)def << "=" << skin.stattrak_count << "\n";
        }
    }

    void load_stattrak() {
        char path[256];
        stattrak_path(path, sizeof(path));
        std::ifstream f(path);
        if (!f.is_open()) return;

        std::string line;
        while (std::getline(f, line)) {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = line.substr(0, eq);
            int v = 0;
            try { v = std::stoi(line.substr(eq + 1)); } catch (...) { continue; }

            if (k == "knife") cfg.equipped_knife_skin.stattrak_count = v;
            else if (k == "glove") cfg.equipped_glove_skin.stattrak_count = v;
            else if (k.size() > 1 && k[0] == 'w') {
                int16_t def = 0;
                try { def = (int16_t)std::stoi(k.substr(1)); } catch (...) {}
                if (def > 0) {
                    auto it = cfg.weapon_skins.find(def);
                    if (it != cfg.weapon_skins.end())
                        it->second.stattrak_count = v;
                }
            }
        }
    }

    void update() {
        if (!cfg.enabled) return;

        uintptr_t local_ctrl = valve::entity::get_local_player_controller();
        uintptr_t local_pawn = valve::entity::get_local_player_pawn();
        if (!local_ctrl || !local_pawn) {
            last_match_kills = -1;
            s_applied_handles.clear();
            s_hud_synced_handles.clear();
            s_last_active_weapon = 0;
            return;
        }

        if (valve::entity::get_life_state(local_pawn) != 0 || valve::entity::get_health(local_pawn) <= 0) {
            s_applied_handles.clear();
            s_hud_synced_handles.clear();
            s_last_active_weapon = 0;
            return;
        }

        if (s_last_pawn != local_pawn) {
            s_applied_handles.clear();
            s_hud_synced_handles.clear();
            s_last_active_weapon = 0;
            s_last_pawn = local_pawn;
        }

        static uint32_t off_pWeaponServices = 0;
        static uint32_t off_hMyWeapons = 0;
        static uint32_t off_hActiveWeapon = 0;
        static uint32_t off_AttributeManager = 0;
        static uint32_t off_nFallbackPaintKit = 0;
        static uint32_t off_nFallbackSeed = 0;
        static uint32_t off_flFallbackWear = 0;
        static uint32_t off_nFallbackStatTrak = 0;
        static uint32_t off_bAttributesInitialized = 0;
        static uint32_t off_iItemDefinitionIndex = 0;
        static uint32_t off_iItemIDHigh = 0;
        static uint32_t off_iItemIDLow = 0;
        static uint32_t off_EconGloves = 0;
        static uint32_t off_bNeedToReApplyGloves = 0;
        static uint32_t off_nEconGlovesChanged = 0;
        static uint32_t off_iPawnKills = 0;
        static uint32_t off_bDisallowSOC = 0;
        static uint32_t off_nSubclassID = 0;
        static uint32_t off_pGameSceneNode = 0;
        static uint32_t off_hHudModelArms = 0;
        static uint32_t off_hViewmodelAttachment = 0;
        static uint32_t off_pChild = 0;
        static uint32_t off_pNextSibling = 0;
        static uint32_t off_pOwner = 0;
        static uint32_t off_m_modelState = 0;
        static uint32_t off_m_MeshGroupMask = 0;
        static uint32_t off_m_Item = 0;
        static uint32_t off_m_ModelName = 0;
        static uint32_t off_m_steamID = 0;
        static uint32_t off_iAccountID = 0;
        static uint32_t off_iEntityQuality = 0;
        static uint32_t off_iQualityOverride = 0;
        static uint32_t off_bInitialized = 0;
        static uint32_t off_bRestoreCustomMaterialAfterPrecache = 0;
        static uint32_t off_iItemID = 0;
        static uint32_t off_OriginalOwnerXuidLow = 0;
        static uint32_t off_OriginalOwnerXuidHigh = 0;
        static uint32_t off_bAttachmentDirty = 0;

        if (!off_pWeaponServices) off_pWeaponServices = schema::lookup("C_BasePlayerPawn", fnv1a::runtime_hash("m_pWeaponServices"));
        if (!off_hMyWeapons) off_hMyWeapons = schema::lookup("CPlayer_WeaponServices", fnv1a::runtime_hash("m_hMyWeapons"));
        if (!off_hActiveWeapon) off_hActiveWeapon = schema::lookup("CPlayer_WeaponServices", fnv1a::runtime_hash("m_hActiveWeapon"));
        if (!off_AttributeManager) off_AttributeManager = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_AttributeManager"));
        if (!off_nFallbackPaintKit) off_nFallbackPaintKit = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_nFallbackPaintKit"));
        if (!off_nFallbackSeed) off_nFallbackSeed = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_nFallbackSeed"));
        if (!off_flFallbackWear) off_flFallbackWear = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_flFallbackWear"));
        if (!off_nFallbackStatTrak) off_nFallbackStatTrak = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_nFallbackStatTrak"));
        if (!off_bAttributesInitialized) off_bAttributesInitialized = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_bAttributesInitialized"));
        if (!off_bDisallowSOC) off_bDisallowSOC = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_bDisallowSOC"));
        if (!off_iItemDefinitionIndex) off_iItemDefinitionIndex = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iItemDefinitionIndex"));
        if (!off_iItemIDHigh) off_iItemIDHigh = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iItemIDHigh"));
        if (!off_iItemIDLow) off_iItemIDLow = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iItemIDLow"));
        if (!off_EconGloves) off_EconGloves = schema::lookup("C_CSPlayerPawn", fnv1a::runtime_hash("m_EconGloves"));
        if (!off_bNeedToReApplyGloves) off_bNeedToReApplyGloves = schema::lookup("C_CSPlayerPawn", fnv1a::runtime_hash("m_bNeedToReApplyGloves"));
        if (!off_nEconGlovesChanged) off_nEconGlovesChanged = schema::lookup("C_CSPlayerPawn", fnv1a::runtime_hash("m_nEconGlovesChanged"));
        if (!off_iPawnKills) off_iPawnKills = schema::lookup("CBasePlayerController", fnv1a::runtime_hash("m_iPawnKills"));
        if (!off_nSubclassID) off_nSubclassID = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_nSubclassID"));
        if (!off_pGameSceneNode) off_pGameSceneNode = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pGameSceneNode"));
        if (!off_hHudModelArms) off_hHudModelArms = schema::lookup("C_CSPlayerPawn", fnv1a::runtime_hash("m_hHudModelArms"));
        if (!off_hViewmodelAttachment) off_hViewmodelAttachment = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_hViewmodelAttachment"));
        if (!off_pChild) off_pChild = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_pChild"));
        if (!off_pNextSibling) off_pNextSibling = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_pNextSibling"));
        if (!off_pOwner) off_pOwner = schema::lookup("CGameSceneNode", fnv1a::runtime_hash("m_pOwner"));
        if (!off_m_modelState) off_m_modelState = schema::lookup("CSkeletonInstance", fnv1a::runtime_hash("m_modelState"));
        if (!off_m_MeshGroupMask) off_m_MeshGroupMask = schema::lookup("CModelState", fnv1a::runtime_hash("m_MeshGroupMask"));
        if (!off_m_Item) off_m_Item = schema::lookup("C_AttributeContainer", fnv1a::runtime_hash("m_Item"));
        if (!off_m_ModelName) off_m_ModelName = schema::lookup("CModelState", fnv1a::runtime_hash("m_ModelName"));
        if (!off_m_steamID) off_m_steamID = schema::lookup("CBasePlayerController", fnv1a::runtime_hash("m_steamID"));
        if (!off_iAccountID) off_iAccountID = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iAccountID"));
        if (!off_iEntityQuality) off_iEntityQuality = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iEntityQuality"));
        if (!off_iQualityOverride) off_iQualityOverride = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iQualityOverride"));
        if (!off_bInitialized) off_bInitialized = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_bInitialized"));
        if (!off_bRestoreCustomMaterialAfterPrecache) off_bRestoreCustomMaterialAfterPrecache = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_bRestoreCustomMaterialAfterPrecache"));
        if (!off_iItemID) off_iItemID = schema::lookup("C_EconItemView", fnv1a::runtime_hash("m_iItemID"));
        if (!off_OriginalOwnerXuidLow) off_OriginalOwnerXuidLow = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_OriginalOwnerXuidLow"));
        if (!off_OriginalOwnerXuidHigh) off_OriginalOwnerXuidHigh = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_OriginalOwnerXuidHigh"));
        if (!off_bAttachmentDirty) off_bAttachmentDirty = schema::lookup("C_EconEntity", fnv1a::runtime_hash("m_bAttachmentDirty"));

        if (!off_pWeaponServices) off_pWeaponServices = 0x1208;
        if (!off_hMyWeapons) off_hMyWeapons = 0x48;
        if (!off_hActiveWeapon) off_hActiveWeapon = 0x58;
        if (!off_AttributeManager) off_AttributeManager = 0x11A8;
        if (!off_nFallbackPaintKit) off_nFallbackPaintKit = 0x1680;
        if (!off_nFallbackSeed) off_nFallbackSeed = 0x1684;
        if (!off_flFallbackWear) off_flFallbackWear = 0x1688;
        if (!off_nFallbackStatTrak) off_nFallbackStatTrak = 0x168C;
        if (!off_bAttributesInitialized) off_bAttributesInitialized = 0x11A0;
        if (!off_bDisallowSOC) off_bDisallowSOC = 0x1E9;
        if (!off_iItemDefinitionIndex) off_iItemDefinitionIndex = 0x1BA;
        if (!off_iItemIDHigh) off_iItemIDHigh = 0x1D0;
        if (!off_iItemIDLow) off_iItemIDLow = 0x1D4;
        if (!off_EconGloves) off_EconGloves = 0x1750;
        if (!off_bNeedToReApplyGloves) off_bNeedToReApplyGloves = 0x1780;
        if (!off_nSubclassID) off_nSubclassID = 0x380;
        if (!off_pGameSceneNode) off_pGameSceneNode = 0x328;
        if (!off_m_Item) off_m_Item = 0x50;
        if (!off_m_ModelName) off_m_ModelName = 0xA0;
        if (!off_m_steamID) off_m_steamID = 0x780;
        if (!off_iAccountID) off_iAccountID = 0x1D8;
        if (!off_iEntityQuality) off_iEntityQuality = 0x1BC;
        if (!off_iQualityOverride) off_iQualityOverride = 0x1F4;
        if (!off_bInitialized) off_bInitialized = 0x1E8;
        if (!off_bRestoreCustomMaterialAfterPrecache) off_bRestoreCustomMaterialAfterPrecache = 0x1B8;
        if (!off_iItemID) off_iItemID = 0x1C8;
        if (!off_OriginalOwnerXuidLow) off_OriginalOwnerXuidLow = 0x1678;
        if (!off_OriginalOwnerXuidHigh) off_OriginalOwnerXuidHigh = 0x167C;
        if (!off_bAttachmentDirty) off_bAttachmentDirty = 0x16B8;

        uint64_t steam_id = (local_ctrl && off_m_steamID) ? memory::read<uint64_t>(local_ctrl + off_m_steamID) : 0;
        uint32_t account_id = static_cast<uint32_t>(steam_id & 0xFFFFFFFF);

        if (off_iPawnKills) {
            int kills_now = memory::read<int>(local_ctrl + off_iPawnKills);
            if (last_match_kills >= 0 && kills_now > last_match_kills) {
                int delta = kills_now - last_match_kills;
                uintptr_t ws_temp = memory::read<uintptr_t>(local_pawn + off_pWeaponServices);
                if (ws_temp) {
                    uint32_t active_h = memory::read<uint32_t>(ws_temp + off_hActiveWeapon);
                    uintptr_t active_w = valve::entity::get_entity_by_handle(active_h);
                    if (active_w) {
                        uintptr_t iv = active_w + off_AttributeManager + off_m_Item;
                        int16_t adef = memory::read<int16_t>(iv + off_iItemDefinitionIndex);
                        bool aknife = (adef >= 500 && adef <= 526) || adef == 42 || adef == 59;
                        if (aknife && cfg.equipped_knife_skin.stattrak) {
                            cfg.equipped_knife_skin.stattrak_count += delta;
                            save_stattrak();
                            s_applied_handles.erase(active_h);
                            s_hud_synced_handles.erase(active_h);
                        } else {
                            auto it = cfg.weapon_skins.find(adef);
                            if (it != cfg.weapon_skins.end() && it->second.stattrak) {
                                it->second.stattrak_count += delta;
                                save_stattrak();
                                s_applied_handles.erase(active_h);
                                s_hud_synced_handles.erase(active_h);
                            }
                        }
                    }
                }
            }
            last_match_kills = kills_now;
        }

        static auto fn_set_attribute = reinterpret_cast<void(__fastcall*)(uintptr_t, const char*, float)>(
            memory::pattern_scan("client.dll", "40 53 48 83 EC 20 48 8B D9 48 81 C1 08 02 00 00"));
        static auto fn_update_skin = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(
            memory::pattern_scan("client.dll", "40 55 53 41 57 48 8D AC 24 00 FE FF FF 48 81 EC 00 03 00 00 44 0F B6 FA 48 8B D9"));
        static auto fn_update_composite = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(
            memory::pattern_scan("client.dll", "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 56 41 57 48 83 EC 20 44 0F B6 F2 48 8B F9"));
        static auto fn_invalidate_desc = reinterpret_cast<void(__fastcall*)(uintptr_t)>(
            memory::pattern_scan("client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8D B9 ? ? ? ? 48 8B F1"));
        static auto fn_weapon_get_viewmodel = reinterpret_cast<void(__fastcall*)(uintptr_t)>(
            memory::pattern_scan("client.dll", "40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 83 BB 88 03 00 00 00"));
        static auto fn_set_model = reinterpret_cast<void(__fastcall*)(uintptr_t, const char*)>(
            []() -> uintptr_t {
                uintptr_t at = memory::pattern_scan("client.dll", "48 8D 15 ? ? ? ? 48 8B CB E8 ? ? ? ? 48 8B D7 48 8B CB");
                if (!at) return 0;
                return memory::resolve_rip(at + 10, 1, 5);
            }());
        static auto fn_weapon_get_model_path = reinterpret_cast<const char*(__fastcall*)(uintptr_t)>(
            memory::pattern_scan("client.dll", "48 89 5C 24 10 56 48 83 EC 20 48 8B 1D ? ? ? ?"));
        static uint32_t off_bVisualsDataSet = schema::lookup("C_CSWeaponBase", fnv1a::runtime_hash("m_bVisualsDataSet"));

        auto usable_addr = [](uintptr_t addr) -> bool {
            if (addr < 0x10000ull) return false;
            if ((addr >> 48) != 0) return false;
            if ((addr & 7ull) != 0) return false;
            if ((addr & 0xffffffffull) == 0) return false;
            if ((addr >> 32) == 0) return false;
            return true;
        };

        auto entity_model_ready = [&](uintptr_t entity) -> bool {
            if (!usable_addr(entity) || !off_pGameSceneNode) return false;
            uintptr_t scene = memory::read<uintptr_t>(entity + off_pGameSceneNode);
            if (!usable_addr(scene) || !off_m_modelState) return false;
            uintptr_t model = memory::read<uintptr_t>(scene + off_m_modelState);
            if (!usable_addr(model)) return false;
            uintptr_t resolved = memory::read<uintptr_t>(model);
            return usable_addr(resolved);
        };

        auto scene_has_model = [&](uintptr_t scene_node) -> bool {
            if (!usable_addr(scene_node) || !off_m_modelState) return false;
            uintptr_t model = memory::read<uintptr_t>(scene_node + off_m_modelState);
            return usable_addr(model);
        };

        auto find_hud_arms = [&](uintptr_t pawn) -> uintptr_t {
            if (!pawn || !usable_addr(pawn) || !off_hHudModelArms) return 0;
            uint32_t handle = memory::read<uint32_t>(pawn + off_hHudModelArms);
            return handle ? valve::entity::get_entity_by_handle(handle) : 0;
        };

        auto entity_model_path = [&](uintptr_t entity) -> std::string {
            if (!entity || !usable_addr(entity) || !off_pGameSceneNode) return {};
            uintptr_t scene = memory::read<uintptr_t>(entity + off_pGameSceneNode);
            if (!usable_addr(scene) || !off_m_modelState) return {};
            uintptr_t model_state = scene + off_m_modelState;
            if (!off_m_ModelName) return {};
            uintptr_t name_ptr = memory::read<uintptr_t>(model_state + off_m_ModelName);
            if (!name_ptr) return {};
            return memory::read_string(name_ptr);
        };

        auto model_stem = [](const char* path) -> const char* {
            if (!path || !*path) return path;
            const char* stem = path;
            for (const char* p = path; *p; ++p) {
                if (*p == '/' || *p == '\\') stem = p + 1;
            }
            return stem;
        };

        auto model_stem_matches = [&](const char* current, const char* expected) -> bool {
            if (!current || !*current || !expected || !*expected) return false;
            const auto a = model_stem(current);
            const auto b = model_stem(expected);
            if (!a || !b || !*b) return false;
            const auto n = strlen(b);
            return strncmp(a, b, n) == 0;
        };

        auto entity_uses_model = [&](uintptr_t entity, const char* expected) -> bool {
            if (!entity || !usable_addr(entity) || !expected || !*expected) return false;
            std::string path = entity_model_path(entity);
            return !path.empty() && model_stem_matches(path.c_str(), expected);
        };

        auto get_viewmodel_attachment = [&](uintptr_t w) -> uintptr_t {
            if (!w || !usable_addr(w) || !off_hViewmodelAttachment) return 0;
            uint32_t handle = memory::read<uint32_t>(w + off_hViewmodelAttachment);
            return handle ? valve::entity::get_entity_by_handle(handle) : 0;
        };

        auto find_hud_weapon = [&](uintptr_t pawn, uintptr_t weapon = 0) -> uintptr_t {
            if (!pawn || !usable_addr(pawn)) return 0;
            uintptr_t arms = find_hud_arms(pawn);
            if (!arms || !usable_addr(arms) || !off_pGameSceneNode || !off_pChild || !off_pOwner || !off_pNextSibling) return 0;
            uintptr_t scene = memory::read<uintptr_t>(arms + off_pGameSceneNode);
            if (!usable_addr(scene)) return 0;

            std::string expected = weapon ? entity_model_path(weapon) : std::string{};
            uintptr_t first = 0;
            uintptr_t matched = 0;

            auto traverse = [&](auto& self, uintptr_t node, int depth) -> void {
                if (!node || !usable_addr(node) || depth > 8 || matched) return;
                uintptr_t child = memory::read<uintptr_t>(node + off_pChild);
                while (child && usable_addr(child) && !matched) {
                    uintptr_t owner = memory::read<uintptr_t>(child + off_pOwner);
                    if (owner && usable_addr(owner)) {
                        const char* name = valve::entity::get_schema_name(owner);
                        if (name && fnv1a::runtime_hash(name) == fnv1a::runtime_hash("C_CS2HudModelWeapon")) {
                            if (!first) first = owner;
                            if (!expected.empty() && entity_uses_model(owner, expected.c_str())) {
                                matched = owner;
                                return;
                            }
                        }
                    }
                    self(self, child, depth + 1);
                    child = memory::read<uintptr_t>(child + off_pNextSibling);
                }
            };
            traverse(traverse, scene, 0);

            if (matched) return matched;
            if (weapon) return 0;
            return first;
        };

        auto find_hud_weapon_for_path = [&](uintptr_t pawn, const char* path) -> uintptr_t {
            if (!pawn || !usable_addr(pawn) || !path || !*path) return 0;
            uintptr_t arms = find_hud_arms(pawn);
            if (!arms || !usable_addr(arms) || !off_pGameSceneNode || !off_pChild || !off_pOwner || !off_pNextSibling) return 0;
            uintptr_t scene = memory::read<uintptr_t>(arms + off_pGameSceneNode);
            if (!usable_addr(scene)) return 0;

            uintptr_t matched = 0;
            auto traverse = [&](auto& self, uintptr_t node, int depth) -> void {
                if (!node || !usable_addr(node) || depth > 8 || matched) return;
                uintptr_t child = memory::read<uintptr_t>(node + off_pChild);
                while (child && usable_addr(child) && !matched) {
                    uintptr_t owner = memory::read<uintptr_t>(child + off_pOwner);
                    if (owner && usable_addr(owner)) {
                        const char* name = valve::entity::get_schema_name(owner);
                        if (name && fnv1a::runtime_hash(name) == fnv1a::runtime_hash("C_CS2HudModelWeapon")) {
                            if (entity_uses_model(owner, path)) {
                                matched = owner;
                                return;
                            }
                        }
                    }
                    self(self, child, depth + 1);
                    child = memory::read<uintptr_t>(child + off_pNextSibling);
                }
            };
            traverse(traverse, scene, 0);
            return matched;
        };

        auto invalidate_composites = [&](uintptr_t entity) {
            if (!entity || !usable_addr(entity) || !entity_model_ready(entity)) return;
            if (fn_update_composite) fn_update_composite(entity + 0x608, true);
        };

        auto set_mesh_group_mask = [&](uintptr_t entity, uint64_t mask) {
            if (!entity || !usable_addr(entity) || !off_pGameSceneNode) return;
            uintptr_t scene = memory::read<uintptr_t>(entity + off_pGameSceneNode);
            if (!scene_has_model(scene) || !off_m_MeshGroupMask) return;
            memory::write<uint64_t>(scene + off_m_modelState + off_m_MeshGroupMask, mask);
        };

        static std::atomic<uint32_t> s_item_serial{ 0x1000 };
        auto write_item_identity = [&](uintptr_t iv, uint32_t acc_id, int quality) {
            if (!iv || !usable_addr(iv)) return;
            uint32_t serial = s_item_serial.fetch_add(1, std::memory_order_relaxed);
            if (off_iItemIDHigh) memory::write<uint32_t>(iv + off_iItemIDHigh, 0xFFFFFFFFu);
            if (off_iItemIDLow) memory::write<uint32_t>(iv + off_iItemIDLow, serial);
            if (off_iItemID) memory::write<uint64_t>(iv + off_iItemID, (0xFFFFFFFFull << 32) | static_cast<uint64_t>(serial));
            if (off_iAccountID) memory::write<uint32_t>(iv + off_iAccountID, acc_id);
            if (off_bInitialized) memory::write<bool>(iv + off_bInitialized, true);
            if (off_bRestoreCustomMaterialAfterPrecache) memory::write<bool>(iv + off_bRestoreCustomMaterialAfterPrecache, true);
            if (off_bDisallowSOC) memory::write<bool>(iv + off_bDisallowSOC, true);
            if (off_iEntityQuality) memory::write<int32_t>(iv + off_iEntityQuality, quality);
        };

        auto apply_weapon_mesh_mask = [&](uintptr_t w, uintptr_t pawn, bool legacy) {
            if (!w || !usable_addr(w)) return;
            uint64_t mask = legacy ? 2ull : 1ull;
            set_mesh_group_mask(w, mask);
            uintptr_t attach = get_viewmodel_attachment(w);
            if (attach && usable_addr(attach)) {
                const char* aname = valve::entity::get_schema_name(attach);
                bool is_st = aname && (strstr(aname, "Stattrak") || strstr(aname, "Module") || strstr(aname, "Addon"));
                if (!is_st) set_mesh_group_mask(attach, mask);
            }

            if (!pawn || !usable_addr(pawn)) return;
            uintptr_t arms = find_hud_arms(pawn);
            if (!arms || !usable_addr(arms) || !off_pGameSceneNode || !off_pChild || !off_pOwner || !off_pNextSibling) return;
            uintptr_t scene = memory::read<uintptr_t>(arms + off_pGameSceneNode);
            if (!usable_addr(scene)) return;

            auto traverse = [&](auto& self, uintptr_t node, int depth) -> void {
                if (!node || !usable_addr(node) || depth > 8) return;
                uintptr_t child = memory::read<uintptr_t>(node + off_pChild);
                while (child && usable_addr(child)) {
                    uintptr_t owner = memory::read<uintptr_t>(child + off_pOwner);
                    if (owner && usable_addr(owner)) {
                        const char* name = valve::entity::get_schema_name(owner);
                        uint32_t hash = name ? fnv1a::runtime_hash(name) : 0u;
                        if (hash == fnv1a::runtime_hash("C_CS2HudModelWeapon")) {
                            if (scene_has_model(child) && off_m_MeshGroupMask) {
                                memory::write<uint64_t>(child + off_m_modelState + off_m_MeshGroupMask, mask);
                            }
                            set_mesh_group_mask(owner, mask);
                        } else if (hash == fnv1a::runtime_hash("C_StattrakModule")) {
                            uintptr_t iv = w + off_AttributeManager + off_m_Item;
                            int16_t def = memory::read<int16_t>(iv + off_iItemDefinitionIndex);
                            bool is_k = (def >= 500 && def <= 526) || def == 42 || def == 59;
                            memory::write<bool>(owner + 0x1188, is_k);
                        }
                    }
                    self(self, child, depth + 1);
                    child = memory::read<uintptr_t>(child + off_pNextSibling);
                }
            };
            traverse(traverse, scene, 0);
        };

        auto refresh_weapon_visuals = [&](uintptr_t weapon, uintptr_t pawn) {
            if (!weapon || !usable_addr(weapon) || !entity_model_ready(weapon)) return;

            if (off_bVisualsDataSet) memory::write<bool>(weapon + off_bVisualsDataSet, false);
            invalidate_composites(weapon);
            if (fn_update_skin) fn_update_skin(weapon, true);

            if (pawn && usable_addr(pawn)) {
                uintptr_t hud = find_hud_weapon(pawn, weapon);
                if (hud && usable_addr(hud)) {
                    invalidate_composites(hud);
                }
            }
        };

        auto apply_knife_models = [&](uintptr_t w, uintptr_t pawn, const char* path, bool apply_hud) {
            if (!path || !*path || !fn_set_model || !w || !usable_addr(w)) return;

            if (!entity_uses_model(w, path)) fn_set_model(w, path);
            uintptr_t attach = get_viewmodel_attachment(w);
            if (attach && usable_addr(attach)) {
                const char* aname = valve::entity::get_schema_name(attach);
                bool is_st = aname && (strstr(aname, "Stattrak") || strstr(aname, "Module") || strstr(aname, "Addon"));
                if (!is_st && !entity_uses_model(attach, path)) fn_set_model(attach, path);
            }

            if (apply_hud && pawn && usable_addr(pawn)) {
                uintptr_t hud = find_hud_weapon_for_path(pawn, path);
                if (!hud) hud = find_hud_weapon(pawn, w);
                if (!hud) hud = find_hud_weapon(pawn, 0);
                if (hud && usable_addr(hud)) {
                    if (!entity_uses_model(hud, path)) fn_set_model(hud, path);
                    invalidate_composites(hud);
                }
            }
        };

        if (!entity_model_ready(local_pawn)) {
            s_applied_handles.clear();
            s_hud_synced_handles.clear();
            s_last_active_weapon = 0;
            return;
        }

        uintptr_t weapon_services = memory::read<uintptr_t>(local_pawn + off_pWeaponServices);
        if (weapon_services && usable_addr(weapon_services)) {
            uint32_t active_handle = memory::read<uint32_t>(weapon_services + off_hActiveWeapon);
            uintptr_t active_weapon = active_handle ? valve::entity::get_entity_by_handle(active_handle) : 0;

            if (active_handle != s_last_active_weapon) {
                s_last_active_weapon = active_handle;
                if (active_weapon && usable_addr(active_weapon) && entity_model_ready(active_weapon)) {
                    uintptr_t iv = active_weapon + off_AttributeManager + off_m_Item;
                    int16_t adef = memory::read<int16_t>(iv + off_iItemDefinitionIndex);
                    bool is_k = (adef >= 500 && adef <= 526) || adef == 42 || adef == 59;
                    if (is_k && cfg.equipped_knife_def > 0) {
                        const char* path = fn_weapon_get_model_path ? fn_weapon_get_model_path(iv) : nullptr;
                        if (path && *path) {
                            apply_knife_models(active_weapon, local_pawn, path, true);
                            apply_weapon_mesh_mask(active_weapon, local_pawn, false);
                        }
                    }
                    if (fn_weapon_get_viewmodel) fn_weapon_get_viewmodel(active_weapon);
                    refresh_weapon_visuals(active_weapon, local_pawn);
                }
            }

            int weapons_size = memory::read<int>(weapon_services + off_hMyWeapons);
            uintptr_t weapons_data = memory::read<uintptr_t>(weapon_services + off_hMyWeapons + 0x8);
            if (weapons_data && usable_addr(weapons_data) && weapons_size > 0 && weapons_size <= 64) {
                for (int i = 0; i < weapons_size; ++i) {
                    uint32_t handle = memory::read<uint32_t>(weapons_data + i * sizeof(uint32_t));
                    if (!handle) continue;
                    uintptr_t weapon = valve::entity::get_entity_by_handle(handle);
                    if (!weapon || !usable_addr(weapon)) continue;

                    if (!entity_model_ready(weapon)) continue;

                    uintptr_t item_view = weapon + off_AttributeManager + off_m_Item;
                    int16_t def_index = memory::read<int16_t>(item_view + off_iItemDefinitionIndex);

                    bool is_knife = (def_index >= 500 && def_index <= 526) || def_index == 42 || def_index == 59;
                    bool is_active = (weapon == active_weapon);

                    if (is_knife && cfg.equipped_knife_def > 0) {
                        auto& ks = cfg.equipped_knife_skin;
                        auto ap_it = s_applied_handles.find(handle);
                        int target_st = ks.stattrak ? (std::max)(0, ks.stattrak_count) : -1;
                        int cur_pk = memory::read<int>(weapon + off_nFallbackPaintKit);
                        int cur_st = memory::read<int>(weapon + off_nFallbackStatTrak);
                        int16_t cur_def = memory::read<int16_t>(item_view + off_iItemDefinitionIndex);

                        bool already = (ap_it != s_applied_handles.end()
                            && ap_it->second.paint_kit == ks.paint_kit_id
                            && ap_it->second.seed == ks.seed
                            && ap_it->second.wear == ks.wear
                            && ap_it->second.stattrak == target_st
                            && ap_it->second.def == cfg.equipped_knife_def
                            && cur_def == cfg.equipped_knife_def
                            && cur_pk == ks.paint_kit_id
                            && cur_st == target_st);

                        if (!already) {
                            write_item_identity(item_view, account_id, 3);
                            memory::write<int16_t>(item_view + off_iItemDefinitionIndex, cfg.equipped_knife_def);

                            memory::write<bool>(weapon + off_bAttributesInitialized, true);
                            if (off_OriginalOwnerXuidLow) memory::write<uint32_t>(weapon + off_OriginalOwnerXuidLow, account_id);
                            if (off_OriginalOwnerXuidHigh) memory::write<uint32_t>(weapon + off_OriginalOwnerXuidHigh, static_cast<uint32_t>(steam_id >> 32));
                            memory::write<int>(weapon + off_nFallbackPaintKit, ks.paint_kit_id);
                            memory::write<int>(weapon + off_nFallbackSeed, (std::max)(1, ks.seed));
                            memory::write<float>(weapon + off_flFallbackWear, ks.wear);
                            memory::write<int>(weapon + off_nFallbackStatTrak, target_st);
                            if (off_bAttachmentDirty) memory::write<bool>(weapon + off_bAttachmentDirty, true);

                            uintptr_t attach = get_viewmodel_attachment(weapon);
                            if (attach && usable_addr(attach)) {
                                if (off_OriginalOwnerXuidLow) memory::write<uint32_t>(attach + off_OriginalOwnerXuidLow, account_id);
                                if (off_OriginalOwnerXuidHigh) memory::write<uint32_t>(attach + off_OriginalOwnerXuidHigh, static_cast<uint32_t>(steam_id >> 32));
                                if (off_nFallbackStatTrak) memory::write<int>(attach + off_nFallbackStatTrak, target_st);
                                if (off_bAttachmentDirty) memory::write<bool>(attach + off_bAttachmentDirty, true);
                            }

                            uint32_t token = make_subclass_token(cfg.equipped_knife_def);
                            memory::write<uint32_t>(weapon + off_nSubclassID, token);
                            if (fn_weapon_get_viewmodel) fn_weapon_get_viewmodel(weapon);

                            const char* model_path = fn_weapon_get_model_path ? fn_weapon_get_model_path(item_view) : nullptr;
                            if (model_path && *model_path) {
                                apply_knife_models(weapon, local_pawn, model_path, is_active);
                            }
                            apply_weapon_mesh_mask(weapon, local_pawn, false);

                            if (fn_set_attribute) {
                                if (ks.paint_kit_id > 0) {
                                    fn_set_attribute(item_view, "set item texture prefab", (float)ks.paint_kit_id);
                                    fn_set_attribute(item_view, "set item texture seed", (float)(std::max)(1, ks.seed));
                                    fn_set_attribute(item_view, "set item texture wear", ks.wear);
                                }
                                if (target_st >= 0) {
                                    fn_set_attribute(item_view, "kill eater", static_cast<float>(target_st));
                                    fn_set_attribute(item_view, "kill eater score type", 0.0f);
                                }
                            }
                            if (fn_invalidate_desc) fn_invalidate_desc(item_view);
                            refresh_weapon_visuals(weapon, is_active ? local_pawn : 0);

                            s_applied_handles[handle] = { ks.paint_kit_id, ks.seed, ks.wear, target_st, cfg.equipped_knife_def };
                            if (is_active) s_hud_synced_handles.insert(handle);
                        } else {
                            apply_weapon_mesh_mask(weapon, local_pawn, false);
                            if (is_active && !s_hud_synced_handles.contains(handle)) {
                                const char* model_path = fn_weapon_get_model_path ? fn_weapon_get_model_path(item_view) : nullptr;
                                if (model_path && *model_path) {
                                    apply_knife_models(weapon, local_pawn, model_path, true);
                                }
                                refresh_weapon_visuals(weapon, local_pawn);
                                s_hud_synced_handles.insert(handle);
                            } else if (!is_active) {
                                s_hud_synced_handles.erase(handle);
                            }
                        }
                        continue;
                    }

                    auto it = cfg.weapon_skins.find(def_index);
                    if (it != cfg.weapon_skins.end()) {
                        const auto& skin = it->second;
                        if (skin.paint_kit_id > 0 || skin.stattrak) {
                            auto ap_it = s_applied_handles.find(handle);
                            int target_st = skin.stattrak ? (std::max)(0, skin.stattrak_count) : -1;
                            int cur_pk = memory::read<int>(weapon + off_nFallbackPaintKit);
                            int cur_st = memory::read<int>(weapon + off_nFallbackStatTrak);
                            bool already = (ap_it != s_applied_handles.end()
                                && ap_it->second.paint_kit == skin.paint_kit_id
                                && ap_it->second.seed == skin.seed
                                && ap_it->second.wear == skin.wear
                                && ap_it->second.stattrak == target_st
                                && cur_pk == skin.paint_kit_id
                                && cur_st == target_st);

                            if (!already) {
                                int quality = target_st >= 0 ? 9 : 0;
                                write_item_identity(item_view, account_id, quality);

                                memory::write<bool>(weapon + off_bAttributesInitialized, true);
                                if (off_OriginalOwnerXuidLow) memory::write<uint32_t>(weapon + off_OriginalOwnerXuidLow, account_id);
                                if (off_OriginalOwnerXuidHigh) memory::write<uint32_t>(weapon + off_OriginalOwnerXuidHigh, static_cast<uint32_t>(steam_id >> 32));
                                memory::write<int>(weapon + off_nFallbackPaintKit, skin.paint_kit_id);
                                memory::write<int>(weapon + off_nFallbackSeed, (std::max)(1, skin.seed));
                                memory::write<float>(weapon + off_flFallbackWear, skin.wear);
                                memory::write<int>(weapon + off_nFallbackStatTrak, target_st);
                                if (off_bAttachmentDirty) memory::write<bool>(weapon + off_bAttachmentDirty, true);

                                uintptr_t attach = get_viewmodel_attachment(weapon);
                                if (attach && usable_addr(attach)) {
                                    if (off_OriginalOwnerXuidLow) memory::write<uint32_t>(attach + off_OriginalOwnerXuidLow, account_id);
                                    if (off_OriginalOwnerXuidHigh) memory::write<uint32_t>(attach + off_OriginalOwnerXuidHigh, static_cast<uint32_t>(steam_id >> 32));
                                    if (off_nFallbackStatTrak) memory::write<int>(attach + off_nFallbackStatTrak, target_st);
                                    if (off_bAttachmentDirty) memory::write<bool>(attach + off_bAttachmentDirty, true);
                                }

                                if (fn_weapon_get_viewmodel) fn_weapon_get_viewmodel(weapon);

                                if (fn_set_attribute) {
                                    if (skin.paint_kit_id > 0) {
                                        fn_set_attribute(item_view, "set item texture prefab", (float)skin.paint_kit_id);
                                        fn_set_attribute(item_view, "set item texture seed", (float)(std::max)(1, skin.seed));
                                        fn_set_attribute(item_view, "set item texture wear", skin.wear);
                                    }
                                    if (target_st >= 0) {
                                        fn_set_attribute(item_view, "kill eater", static_cast<float>(target_st));
                                        fn_set_attribute(item_view, "kill eater score type", 0.0f);
                                    }
                                }
                                if (fn_invalidate_desc) fn_invalidate_desc(item_view);
                                refresh_weapon_visuals(weapon, is_active ? local_pawn : 0);
                                apply_weapon_mesh_mask(weapon, local_pawn, false);

                                s_applied_handles[handle] = { skin.paint_kit_id, skin.seed, skin.wear, target_st, def_index };
                                if (is_active) s_hud_synced_handles.insert(handle);
                            } else {
                                apply_weapon_mesh_mask(weapon, local_pawn, false);
                                if (is_active && !s_hud_synced_handles.contains(handle)) {
                                    refresh_weapon_visuals(weapon, local_pawn);
                                    s_hud_synced_handles.insert(handle);
                                } else if (!is_active) {
                                    s_hud_synced_handles.erase(handle);
                                }
                            }
                        }
                    }
                }
            }
        }

        
        if (cfg.equipped_glove_def > 0 && off_EconGloves) {
            uintptr_t giv = local_pawn + off_EconGloves;
            int16_t cur_gdef = memory::read<int16_t>(giv + off_iItemDefinitionIndex);
            int cur_pk = memory::read<int>(giv + 0x1680);
            if (cur_gdef != cfg.equipped_glove_def || cur_pk != cfg.equipped_glove_skin.paint_kit_id) {
                memory::write<bool>(giv + 0x1A0, true);
                memory::write<bool>(giv + off_bDisallowSOC, true);
                memory::write<uint32_t>(giv + off_iItemIDHigh, 0xFFFFFFFF);
                memory::write<uint32_t>(giv + off_iItemIDLow, 0);
                memory::write<int16_t>(giv + off_iItemDefinitionIndex, cfg.equipped_glove_def);
                if (cfg.equipped_glove_skin.paint_kit_id > 0) {
                    memory::write<int>(giv + 0x1680, cfg.equipped_glove_skin.paint_kit_id);
                    memory::write<int>(giv + 0x1684, (std::max)(1, cfg.equipped_glove_skin.seed));
                    memory::write<float>(giv + 0x1688, cfg.equipped_glove_skin.wear);
                    if (fn_set_attribute) {
                        fn_set_attribute(giv, "set item texture prefab", (float)cfg.equipped_glove_skin.paint_kit_id);
                        fn_set_attribute(giv, "set item texture seed", (float)(std::max)(1, cfg.equipped_glove_skin.seed));
                        fn_set_attribute(giv, "set item texture wear", cfg.equipped_glove_skin.wear);
                    }
                }
                if (off_bNeedToReApplyGloves) memory::write<bool>(local_pawn + off_bNeedToReApplyGloves, true);
                if (off_nEconGlovesChanged) {
                    int cnt = memory::read<int>(local_pawn + off_nEconGlovesChanged);
                    memory::write<int>(local_pawn + off_nEconGlovesChanged, cnt + 1);
                }
            }
        }
    }

    struct SkinTexture {
        ImTextureID id = (ImTextureID)0;
        int w = 0;
        int h = 0;
    };

    static SkinTexture get_skin_texture(int16_t def, int paint_id) {
        auto* vtex = vpk_vtex::get_skin_image(def, paint_id);
        if (vtex && vtex->srv) {
            return { (ImTextureID)(uintptr_t)vtex->srv, vtex->width, vtex->height };
        }

        const auto* item = EconItemSystem::get().find_item(def);
        if (!item) return {};

        const char* simple_name = item->name.c_str();

        if (paint_id > 0) {
            const auto* pk = EconItemSystem::get().find_paint_kit(paint_id);
            if (pk) {
                ImTextureID id = skin_img::get_paint(simple_name, pk->name.c_str(), pk->id);
                if (id) return { id, 512, 384 };
            }
        }
        ImTextureID id = skin_img::get_weapon(simple_name);
        return { id, 512, 384 };
    }

    enum class CardResult { None, Pressed, Remove };

    static CardResult draw_skin_card(const char* label, SkinTexture tex, bool is_add,
        ImVec2 size, bool show_remove = false)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

        ImGui::InvisibleButton(label, size);
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();

        dl->PushClipRect(pos, max, true);

        ImU32 border_col = hovered ? ImGui::GetColorU32(gui::theme::accent_color) : ImGui::GetColorU32(gui::theme::outline_color);
        ImU32 bg_col = hovered ? IM_COL32(36, 36, 42, 255) : IM_COL32(24, 24, 28, 255);

        dl->AddRectFilled(pos, max, bg_col, 2.0f);
        dl->AddRect(pos, max, border_col, 2.0f);

        if (is_add) {
            ImVec2 ts = ImGui::CalcTextSize("+");
            dl->AddText(ImVec2(pos.x + (size.x - ts.x) * 0.5f, pos.y + (size.y - ts.y) * 0.5f - 8.0f),
                hovered ? ImGui::GetColorU32(gui::theme::accent_color) : IM_COL32(180, 180, 195, 255), "+");
            ImVec2 ls = ImGui::CalcTextSize(label ? label : "Add");
            dl->AddText(ImVec2(pos.x + (size.x - ls.x) * 0.5f, pos.y + (size.y - ts.y) * 0.5f + 12.0f),
                ImGui::GetColorU32(gui::theme::text_dim_color), label ? label : "Add");
        } else {
            if (tex.id) {
                float pad = 10.0f;
                float box_w = size.x - pad * 2.0f;
                float box_h = size.y - 28.0f - pad;

                float tw = (tex.w > 0) ? (float)tex.w : 512.0f;
                float th = (tex.h > 0) ? (float)tex.h : 384.0f;

                float scale = (std::min)(box_w / tw, box_h / th) * 0.82f;
                float draw_w = tw * scale;
                float draw_h = th * scale;

                float draw_x = pos.x + pad + (box_w - draw_w) * 0.5f;
                float draw_y = pos.y + pad + (box_h - draw_h) * 0.5f;

                dl->AddImage(tex.id, ImVec2(draw_x, draw_y), ImVec2(draw_x + draw_w, draw_y + draw_h));
            }

            std::string display_lbl = label ? label : "";
            ImVec2 ts = ImGui::CalcTextSize(display_lbl.c_str());
            if (ts.x > size.x - 8.0f && display_lbl.length() > 4) {
                while (!display_lbl.empty() && ImGui::CalcTextSize((display_lbl + "...").c_str()).x > size.x - 8.0f)
                    display_lbl.pop_back();
                display_lbl += "...";
                ts = ImGui::CalcTextSize(display_lbl.c_str());
            }

            float txt_x = pos.x + (size.x - ts.x) * 0.5f;
            float txt_y = max.y - ts.y - 5.0f;
            if (txt_x < pos.x + 4.0f) txt_x = pos.x + 4.0f;
            gui::theme::draw_text_stroke(dl, display_lbl.c_str(), ImVec2(txt_x, txt_y), ImGui::GetColorU32(gui::theme::text_color));
        }

        dl->PopClipRect();
        return clicked ? CardResult::Pressed : CardResult::None;
    }

    void render_ui() {
        vpk_vtex::tick();
        EconItemSystem::get().initialize();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float grid_spacing = 8.0f;

        
        if (ui_state.state != TabState::Home) {
            if (widgets::button("< Back", 75.0f)) {
                if (ui_state.state == TabState::SelectCategory) ui_state.state = TabState::Home;
                else if (ui_state.state == TabState::SelectItem) ui_state.state = TabState::SelectCategory;
                else if (ui_state.state == TabState::SelectSkin) ui_state.state = TabState::SelectItem;
                else if (ui_state.state == TabState::Preview) ui_state.state = TabState::SelectSkin;
            }
            ImGui::SameLine(0, 10.0f);
            const char* stage_title = "Skins";
            if (ui_state.state == TabState::SelectCategory) stage_title = "Select Category";
            else if (ui_state.state == TabState::SelectItem) stage_title = (ui_state.cat == ItemCategory::Weapons) ? "Select Weapon" : (ui_state.cat == ItemCategory::Knives) ? "Select Knife" : (ui_state.cat == ItemCategory::Gloves) ? "Select Glove" : "Select Agent";
            else if (ui_state.state == TabState::SelectSkin) stage_title = "Select Skin";
            else if (ui_state.state == TabState::Preview) stage_title = "Preview & Customize";

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(gui::theme::text_color, "%s", stage_title);
            ImGui::Spacing();
        }

        
        
        
        if (ui_state.state == TabState::Home) {
            ImGui::BeginChild("##skin_home_grid", ImVec2(avail.x, avail.y - 4.0f), false);

            float avail_w = ImGui::GetContentRegionAvail().x;
            ImVec2 card_size(120.0f, 130.0f);
            int cols = (int)(avail_w / (card_size.x + grid_spacing));
            if (cols < 1) cols = 1;

            int col = 0;
            if (draw_skin_card("Add Skin", {}, true, card_size, false) == CardResult::Pressed) {
                ui_state.state = TabState::SelectCategory;
            }
            col++;

            
            if (cfg.equipped_knife_def > 0) {
                const auto* item = EconItemSystem::get().find_item(cfg.equipped_knife_def);
                const auto* pk = EconItemSystem::get().find_paint_kit(cfg.equipped_knife_skin.paint_kit_id);
                std::string title = item ? item->display_name : "Knife";
                if (pk) title += " | " + pk->display_name;

                SkinTexture tex = get_skin_texture(cfg.equipped_knife_def, cfg.equipped_knife_skin.paint_kit_id);
                if (col > 0 && col < cols) ImGui::SameLine(0, grid_spacing);
                else if (col >= cols) col = 0;

                if (draw_skin_card(title.c_str(), tex, false, card_size, true) == CardResult::Pressed) {
                    ui_state.cat = ItemCategory::Knives;
                    ui_state.selected_def = cfg.equipped_knife_def;
                    ui_state.selected_paint = cfg.equipped_knife_skin.paint_kit_id;
                    ui_state.wear = cfg.equipped_knife_skin.wear;
                    ui_state.seed = cfg.equipped_knife_skin.seed;
                    ui_state.stattrak = cfg.equipped_knife_skin.stattrak;
                    ui_state.stattrak_count = cfg.equipped_knife_skin.stattrak_count;
                    ui_state.state = TabState::Preview;
                }
                col++;
            }

            
            if (cfg.equipped_glove_def > 0) {
                const auto* item = EconItemSystem::get().find_item(cfg.equipped_glove_def);
                const auto* pk = EconItemSystem::get().find_paint_kit(cfg.equipped_glove_skin.paint_kit_id);
                std::string title = item ? item->display_name : "Gloves";
                if (pk) title += " | " + pk->display_name;

                SkinTexture tex = get_skin_texture(cfg.equipped_glove_def, cfg.equipped_glove_skin.paint_kit_id);
                if (col > 0 && col < cols) ImGui::SameLine(0, grid_spacing);
                else if (col >= cols) col = 0;

                if (draw_skin_card(title.c_str(), tex, false, card_size, true) == CardResult::Pressed) {
                    ui_state.cat = ItemCategory::Gloves;
                    ui_state.selected_def = cfg.equipped_glove_def;
                    ui_state.selected_paint = cfg.equipped_glove_skin.paint_kit_id;
                    ui_state.wear = cfg.equipped_glove_skin.wear;
                    ui_state.seed = cfg.equipped_glove_skin.seed;
                    ui_state.stattrak = cfg.equipped_glove_skin.stattrak;
                    ui_state.stattrak_count = cfg.equipped_glove_skin.stattrak_count;
                    ui_state.state = TabState::Preview;
                }
                col++;
            }

            
            for (const auto& [def, skin] : cfg.weapon_skins) {
                if (skin.paint_kit_id <= 0) continue;
                const auto* item = EconItemSystem::get().find_item(def);
                if (!item) continue;

                const auto* pk = EconItemSystem::get().find_paint_kit(skin.paint_kit_id);
                std::string title = item->display_name;
                if (pk) title += " | " + pk->display_name;

                SkinTexture tex = get_skin_texture(def, skin.paint_kit_id);

                if (col > 0 && col < cols) ImGui::SameLine(0, grid_spacing);
                else if (col >= cols) col = 0;

                if (draw_skin_card(title.c_str(), tex, false, card_size, true) == CardResult::Pressed) {
                    ui_state.cat = ItemCategory::Weapons;
                    ui_state.selected_def = def;
                    ui_state.selected_paint = skin.paint_kit_id;
                    ui_state.wear = skin.wear;
                    ui_state.seed = skin.seed;
                    ui_state.stattrak = skin.stattrak;
                    ui_state.stattrak_count = skin.stattrak_count;
                    ui_state.state = TabState::Preview;
                }
                col++;
            }

            ImGui::EndChild();
        }
        
        
        
        else if (ui_state.state == TabState::SelectCategory) {
            ImGui::BeginChild("##skin_cat_grid", ImVec2(avail.x, avail.y - 40.0f), false);

            float cat_card_w = (avail.x - grid_spacing) * 0.5f;
            float cat_card_h = 135.0f;
            ImVec2 cat_size(cat_card_w, cat_card_h);

            
            SkinTexture w_tex = get_skin_texture(7, 0); 
            if (draw_skin_card("Weapons", w_tex, false, cat_size) == CardResult::Pressed) {
                ui_state.cat = ItemCategory::Weapons;
                s_item_search[0] = '\0';
                ui_state.state = TabState::SelectItem;
            }

            ImGui::SameLine(0, grid_spacing);
            SkinTexture k_tex = get_skin_texture(500, 0); 
            if (draw_skin_card("Knives", k_tex, false, cat_size) == CardResult::Pressed) {
                ui_state.cat = ItemCategory::Knives;
                s_item_search[0] = '\0';
                ui_state.state = TabState::SelectItem;
            }

            ImGui::Spacing();

            
            SkinTexture g_tex = get_skin_texture(5027, 0); 
            if (draw_skin_card("Gloves", g_tex, false, cat_size) == CardResult::Pressed) {
                ui_state.cat = ItemCategory::Gloves;
                s_item_search[0] = '\0';
                ui_state.state = TabState::SelectItem;
            }

            ImGui::SameLine(0, grid_spacing);
            SkinTexture a_tex = get_skin_texture(4619, 0); 
            if (draw_skin_card("Agents", a_tex, false, cat_size) == CardResult::Pressed) {
                ui_state.cat = ItemCategory::Agents;
                s_item_search[0] = '\0';
                ui_state.state = TabState::SelectItem;
            }

            ImGui::EndChild();
        }
        
        
        
        else if (ui_state.state == TabState::SelectItem) {
            widgets::textbox("Search Item", s_item_search, sizeof(s_item_search));
            ImGui::Spacing();

            float content_h = ImGui::GetContentRegionAvail().y - 4.0f;
            ImGui::BeginChild("##skin_item_grid", ImVec2(avail.x, content_h), false);

            auto items = EconItemSystem::get().get_items_by_category(ui_state.cat);

            std::string filter_str = s_item_search;
            std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), [](unsigned char c) { return (char)std::tolower(c); });

            float avail_w = ImGui::GetContentRegionAvail().x;
            float item_card_w = (avail_w - grid_spacing * 2.0f) / 3.0f;
            if (item_card_w < 100.0f) item_card_w = 100.0f;
            ImVec2 item_size(item_card_w, 125.0f);

            int cols = 3;
            int col = 0;

            for (const auto* item : items) {
                if (!filter_str.empty()) {
                    std::string item_lower = item->display_name;
                    std::transform(item_lower.begin(), item_lower.end(), item_lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                    if (item_lower.find(filter_str) == std::string::npos)
                        continue;
                }

                SkinTexture tex = get_skin_texture(item->def_index, 0);

                if (col > 0 && col < cols) ImGui::SameLine(0, grid_spacing);
                else if (col >= cols) col = 0;

                if (draw_skin_card(item->display_name.c_str(), tex, false, item_size) == CardResult::Pressed) {
                    ui_state.selected_def = item->def_index;
                    ui_state.selected_paint = 0;
                    ui_state.wear = 0.001f;
                    ui_state.seed = 1;
                    ui_state.stattrak = false;
                    ui_state.stattrak_count = 1337;

                    if (ui_state.cat == ItemCategory::Agents) {
                        if (item->team == 3) cfg.equipped_agent_ct = item->def_index;
                        else if (item->team == 2) cfg.equipped_agent_t = item->def_index;
                        else { cfg.equipped_agent_ct = cfg.equipped_agent_t = item->def_index; }
                        ui_state.state = TabState::Home;
                    } else {
                        s_skin_search[0] = '\0';
                        ui_state.state = TabState::SelectSkin;
                    }
                }
                col++;
            }

            ImGui::EndChild();
        }
        
        
        
        else if (ui_state.state == TabState::SelectSkin) {
            widgets::textbox("Search Skin", s_skin_search, sizeof(s_skin_search));
            ImGui::Spacing();

            float content_h = ImGui::GetContentRegionAvail().y - 4.0f;
            ImGui::BeginChild("##skin_paint_grid", ImVec2(avail.x, content_h), false);

            const auto kits = EconItemSystem::get().get_paint_kits_for_item(ui_state.selected_def);

            std::string filter_str = s_skin_search;
            std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), [](unsigned char c) { return (char)std::tolower(c); });

            float avail_w = ImGui::GetContentRegionAvail().x;
            float skin_card_w = (avail_w - grid_spacing * 2.0f) / 3.0f;
            if (skin_card_w < 100.0f) skin_card_w = 100.0f;
            ImVec2 skin_size(skin_card_w, 125.0f);

            int cols = 3;
            int col = 0;

            for (const auto* pk : kits) {
                if (!pk) continue;
                if (!filter_str.empty()) {
                    std::string pk_lower = pk->display_name;
                    std::transform(pk_lower.begin(), pk_lower.end(), pk_lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                    if (pk_lower.find(filter_str) == std::string::npos)
                        continue;
                }

                SkinTexture tex = get_skin_texture(ui_state.selected_def, pk->id);

                if (col > 0 && col < cols) ImGui::SameLine(0, grid_spacing);
                else if (col >= cols) col = 0;

                if (draw_skin_card(pk->display_name.c_str(), tex, false, skin_size) == CardResult::Pressed) {
                    ui_state.selected_paint = pk->id;
                    ui_state.state = TabState::Preview;
                }
                col++;
            }

            ImGui::EndChild();
        }
        
        
        
        else if (ui_state.state == TabState::Preview) {
            float full_w = ImGui::GetContentRegionAvail().x;
            float col_w = (full_w - 6.0f) * 0.5f;
            float avail_h = ImGui::GetContentRegionAvail().y - 4.0f;

            
            ImGui::BeginGroup();
            if (widgets::begin_section("Preview", col_w, avail_h)) {
                const auto* item = EconItemSystem::get().find_item(ui_state.selected_def);
                const auto* pk = EconItemSystem::get().find_paint_kit(ui_state.selected_paint);

                std::string item_title = item ? item->display_name : "Unknown";
                std::string skin_title = pk ? pk->display_name : "Default";

                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(gui::theme::text_color, "%s | %s", item_title.c_str(), skin_title.c_str());
                ImGui::Spacing();

                float frame_w = col_w - 24.0f;
                float frame_h = frame_w * 0.75f;
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = ImVec2(p0.x + frame_w, p0.y + frame_h);
                ImDrawList* dl = ImGui::GetWindowDrawList();

                dl->AddRectFilled(p0, p1, IM_COL32(22, 22, 26, 255), 2.0f);
                dl->AddRect(p0, p1, ImGui::GetColorU32(gui::theme::outline_color), 2.0f);

                SkinTexture prev_tex = get_skin_texture(ui_state.selected_def, ui_state.selected_paint);
                if (prev_tex.id) {
                    float tw = (prev_tex.w > 0) ? (float)prev_tex.w : 512.0f;
                    float th = (prev_tex.h > 0) ? (float)prev_tex.h : 384.0f;
                    float scale = (std::min)((frame_w - 12.0f) / tw, (frame_h - 12.0f) / th) * 0.90f;
                    float dw = tw * scale;
                    float dh = th * scale;
                    float dx = p0.x + (frame_w - dw) * 0.5f;
                    float dy = p0.y + (frame_h - dh) * 0.5f;
                    dl->AddImage(prev_tex.id, ImVec2(dx, dy), ImVec2(dx + dw, dy + dh));
                } else {
                    ImVec2 txt_sz = ImGui::CalcTextSize("Loading preview...");
                    dl->AddText(ImVec2(p0.x + (frame_w - txt_sz.x) * 0.5f, p0.y + (frame_h - txt_sz.y) * 0.5f),
                        ImGui::GetColorU32(gui::theme::text_dim_color), "Loading preview...");
                }
                ImGui::Dummy(ImVec2(frame_w, frame_h));

                widgets::end_section();
            }
            ImGui::EndGroup();

            ImGui::SameLine(0, 6.0f);

            
            ImGui::BeginGroup();
            if (widgets::begin_section("Customization", col_w, avail_h)) {
                widgets::slider_float("Wear", &ui_state.wear, 0.0001f, 1.0f, "", 4);
                widgets::slider_int("Pattern Seed", &ui_state.seed, 1, 1000);
                // widgets::toggle("StatTrak", &ui_state.stattrak);
                // if (ui_state.stattrak) {
                //     widgets::slider_int("StatTrak Kills", &ui_state.stattrak_count, 0, 999999);
                // }
                // it doesnt work so i disabled it
                // if you are reading this you need to give me money

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (widgets::button("Apply Skin", -1.0f)) {
                    if (ui_state.cat == ItemCategory::Weapons) {
                        AppliedSkin s;
                        s.paint_kit_id = ui_state.selected_paint;
                        s.wear = ui_state.wear;
                        s.seed = ui_state.seed;
                        s.stattrak = ui_state.stattrak;
                        s.stattrak_count = ui_state.stattrak_count;
                        cfg.weapon_skins[ui_state.selected_def] = s;
                    } else if (ui_state.cat == ItemCategory::Knives) {
                        cfg.equipped_knife_def = ui_state.selected_def;
                        cfg.equipped_knife_skin.paint_kit_id = ui_state.selected_paint;
                        cfg.equipped_knife_skin.wear = ui_state.wear;
                        cfg.equipped_knife_skin.seed = ui_state.seed;
                        cfg.equipped_knife_skin.stattrak = ui_state.stattrak;
                        cfg.equipped_knife_skin.stattrak_count = ui_state.stattrak_count;
                    } else if (ui_state.cat == ItemCategory::Gloves) {
                        cfg.equipped_glove_def = ui_state.selected_def;
                        cfg.equipped_glove_skin.paint_kit_id = ui_state.selected_paint;
                        cfg.equipped_glove_skin.wear = ui_state.wear;
                        cfg.equipped_glove_skin.seed = ui_state.seed;
                        cfg.equipped_glove_skin.stattrak = ui_state.stattrak;
                        cfg.equipped_glove_skin.stattrak_count = ui_state.stattrak_count;
                    }
                    save_stattrak();
                    s_applied_handles.clear();
                    ui_state.state = TabState::Home;
                }

                widgets::end_section();
            }
            ImGui::EndGroup();
        }
    }

}

#include "entity.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"
#include "../schema/schema.hpp"

namespace valve::entity {

    static inline bool is_game_ptr(uintptr_t ptr) {
        return ptr >= 0x10000 && ptr < 0x7FFFFFFFFFFF;
    }

    uintptr_t get_entity_by_index(int index) {
        if (!interfaces::entity_list || index < 0 || index >= 0x7FFE) return 0;

        uintptr_t ent_list_ptr = memory::read<uintptr_t>(interfaces::entity_list);
        if (!is_game_ptr(ent_list_ptr)) return 0;

        uint32_t chunk_idx = static_cast<uint32_t>(index) >> 9;
        if (chunk_idx >= 64) return 0;

        uintptr_t list_entry = memory::read<uintptr_t>(ent_list_ptr + (chunk_idx * 8) + 0x10);
        if (!is_game_ptr(list_entry)) return 0;

        uintptr_t identity = list_entry + ((static_cast<uint32_t>(index) & 0x1FF) * 112);
        uintptr_t entity = memory::read<uintptr_t>(identity);
        return is_game_ptr(entity) ? entity : 0;
    }

    uintptr_t get_entity_by_handle(uint32_t handle) {
        if (!handle || handle == 0xFFFFFFFF || handle == 0xFFFFFFFE) return 0;
        uint32_t index = handle & 0x7FFF;
        if (index >= 0x7FFE) return 0;

        if (!interfaces::entity_list) return 0;
        uintptr_t ent_list_ptr = memory::read<uintptr_t>(interfaces::entity_list);
        if (!is_game_ptr(ent_list_ptr)) return 0;

        uint32_t chunk_idx = index >> 9;
        if (chunk_idx >= 64) return 0;

        uintptr_t list_entry = memory::read<uintptr_t>(ent_list_ptr + (chunk_idx * 8) + 0x10);
        if (!is_game_ptr(list_entry)) return 0;

        uintptr_t identity = list_entry + ((index & 0x1FF) * 112);
        if (memory::read<uint32_t>(identity + 0x10) != handle) return 0;

        uintptr_t entity = memory::read<uintptr_t>(identity);
        return is_game_ptr(entity) ? entity : 0;
    }

    const char* get_schema_name(uintptr_t entity) {
        if (!is_game_ptr(entity)) return nullptr;
        uintptr_t identity = memory::read<uintptr_t>(entity + 0x10);
        if (!is_game_ptr(identity)) return nullptr;
        uintptr_t entity_class = memory::read<uintptr_t>(identity + 0x8);
        if (!is_game_ptr(entity_class)) return nullptr;
        uintptr_t class_info = memory::read<uintptr_t>(entity_class + 0x58);
        if (!is_game_ptr(class_info)) return nullptr;
        uintptr_t name_ptr = memory::read<uintptr_t>(class_info + 0x8);
        return is_game_ptr(name_ptr) ? reinterpret_cast<const char*>(name_ptr) : nullptr;
    }

    uintptr_t get_local_player_controller() {
        if (!interfaces::local_player_controller) return 0;
        uintptr_t ctrl = memory::read<uintptr_t>(interfaces::local_player_controller);
        return is_game_ptr(ctrl) ? ctrl : 0;
    }

    uintptr_t get_local_player_pawn() {
        uintptr_t ctrl = get_local_player_controller();
        if (!ctrl) return 0;

        static uint32_t off_hPlayerPawn = 0;
        if (!off_hPlayerPawn) off_hPlayerPawn = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_hPlayerPawn"));
        if (!off_hPlayerPawn) return 0;

        uint32_t pawn_handle = memory::read<uint32_t>(ctrl + off_hPlayerPawn);
        return get_entity_by_handle(pawn_handle);
    }

    uint8_t get_team(uintptr_t entity) {
        if (!is_game_ptr(entity)) return 0;
        static uint32_t off_iTeamNum = 0;
        if (!off_iTeamNum) off_iTeamNum = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_iTeamNum"));
        return off_iTeamNum ? memory::read<uint8_t>(entity + off_iTeamNum) : 0;
    }

    int get_health(uintptr_t entity) {
        if (!is_game_ptr(entity)) return 0;
        static uint32_t off_iHealth = 0;
        if (!off_iHealth) off_iHealth = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_iHealth"));
        return off_iHealth ? memory::read<int>(entity + off_iHealth) : 0;
    }

    uint8_t get_life_state(uintptr_t entity) {
        if (!is_game_ptr(entity)) return 0;
        static uint32_t off_lifeState = 0;
        if (!off_lifeState) off_lifeState = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_lifeState"));
        return off_lifeState ? memory::read<uint8_t>(entity + off_lifeState) : 0;
    }

    std::string get_player_name(uintptr_t controller) {
        if (!is_game_ptr(controller)) return "";

        static uint32_t off_sSanitizedPlayerName = 0;
        static uint32_t off_iszPlayerName = 0;

        if (!off_sSanitizedPlayerName) off_sSanitizedPlayerName = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_sSanitizedPlayerName"));
        if (!off_iszPlayerName) off_iszPlayerName = schema::lookup("CBasePlayerController", fnv1a::runtime_hash("m_iszPlayerName"));

        if (off_sSanitizedPlayerName) {
            uintptr_t name_ptr = memory::read<uintptr_t>(controller + off_sSanitizedPlayerName);
            if (is_game_ptr(name_ptr)) {
                std::string s = memory::read_string(name_ptr, 128);
                if (!s.empty()) return s;
            }
        }

        if (off_iszPlayerName) {
            std::string s = memory::read_string(controller + off_iszPlayerName, 128);
            if (!s.empty()) return s;
        }

        return "";
    }

    std::string get_active_weapon_name(uintptr_t pawn) {
        if (!is_game_ptr(pawn)) return "";

        static uint32_t off_pWeaponServices = 0;
        static uint32_t off_hActiveWeapon = 0;
        static uint32_t off_nSubclassID = 0;
        static uint32_t off_szName = 0;

        if (!off_pWeaponServices) off_pWeaponServices = schema::lookup("C_BasePlayerPawn", fnv1a::runtime_hash("m_pWeaponServices"));
        if (!off_hActiveWeapon) off_hActiveWeapon = schema::lookup("CPlayer_WeaponServices", fnv1a::runtime_hash("m_hActiveWeapon"));
        if (!off_nSubclassID) off_nSubclassID = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_nSubclassID"));
        if (!off_szName) off_szName = schema::lookup("CCSWeaponBaseVData", fnv1a::runtime_hash("m_szName"));
        if (!off_szName) off_szName = schema::lookup("CBasePlayerWeaponVData", fnv1a::runtime_hash("m_szName"));

        if (!off_pWeaponServices || !off_hActiveWeapon) return "";

        uintptr_t weapon_services = memory::read<uintptr_t>(pawn + off_pWeaponServices);
        if (!is_game_ptr(weapon_services)) return "";

        uint32_t weapon_handle = memory::read<uint32_t>(weapon_services + off_hActiveWeapon);
        if (!weapon_handle || weapon_handle == 0xFFFFFFFF || weapon_handle == 0xFFFFFFFE) return "";

        uintptr_t weapon_ent = get_entity_by_handle(weapon_handle);
        if (!is_game_ptr(weapon_ent)) return "";

        uint32_t sub_off = off_nSubclassID ? off_nSubclassID : 0x368;
        uintptr_t vdata = memory::read<uintptr_t>(weapon_ent + sub_off + 0x8);
        if (!is_game_ptr(vdata)) return "";

        uint32_t sz_off = off_szName ? off_szName : 0xC8;
        uintptr_t name_ptr = memory::read<uintptr_t>(vdata + sz_off);
        if (!is_game_ptr(name_ptr)) return "";

        std::string name = memory::read_string(name_ptr, 64);
        if (name.rfind("weapon_", 0) == 0) {
            name.erase(0, 7);
        }
        return name;
    }

    WeaponInfo get_weapon_info(const std::string& raw_name) {
        uint32_t hash = fnv1a::runtime_hash(raw_name.c_str());
        switch (hash) {
            case "ak47"_hash: return { "AK-47", "AK-47" };
            case "m4a1"_hash: return { "M4A4", "M4A4" };
            case "m4a1_silencer"_hash: return { "M4A1-S", "M4A1-S" };
            case "awp"_hash: return { "AWP", "AWP" };
            case "deagle"_hash: return { "Deagle", "Deagle" };
            case "glock"_hash: return { "Glock-18", "Glock-18" };
            case "usp_silencer"_hash: return { "USP-S", "USP-S" };
            case "hkp2000"_hash: return { "P2000", "P2000" };
            case "p250"_hash: return { "P250", "P250" };
            case "cz75a"_hash: return { "CZ75-Auto", "CZ75-Auto" };
            case "fiveseven"_hash: return { "Five-SeveN", "Five-SeveN" };
            case "tec9"_hash: return { "Tec-9", "Tec-9" };
            case "revolver"_hash: return { "Revolver", "Revolver" };
            case "elite"_hash: return { "Dual Berettas", "Dual Berettas" };
            case "mac10"_hash: return { "MAC-10", "MAC-10" };
            case "mp9"_hash: return { "MP9", "MP9" };
            case "mp7"_hash: return { "MP7", "MP7" };
            case "mp5sd"_hash: return { "MP5-SD", "MP5-SD" };
            case "ump45"_hash: return { "UMP-45", "UMP-45" };
            case "p90"_hash: return { "P90", "P90" };
            case "bizon"_hash: return { "PP-Bizon", "PP-Bizon" };
            case "galilar"_hash: return { "Galil AR", "Galil AR" };
            case "famas"_hash: return { "FAMAS", "FAMAS" };
            case "aug"_hash: return { "AUG", "AUG" };
            case "sg556"_hash: return { "SG 553", "SG 553" };
            case "ssg08"_hash: return { "SSG 08", "SSG 08" };
            case "scar20"_hash: return { "SCAR-20", "SCAR-20" };
            case "g3sg1"_hash: return { "G3SG1", "G3SG1" };
            case "nova"_hash: return { "Nova", "Nova" };
            case "xm1014"_hash: return { "XM1014", "XM1014" };
            case "sawedoff"_hash: return { "Sawed-Off", "Sawed-Off" };
            case "mag7"_hash: return { "MAG-7", "MAG-7" };
            case "m249"_hash: return { "M249", "M249" };
            case "negev"_hash: return { "Negev", "Negev" };
            case "hegrenade"_hash: return { "HE Grenade", "HE Grenade" };
            case "flashbang"_hash: return { "Flashbang", "Flashbang" };
            case "smokegrenade"_hash: return { "Smoke", "Smoke" };
            case "molotov"_hash:
            case "incgrenade"_hash: return { "Molotov", "Molotov" };
            case "decoy"_hash: return { "Decoy", "Decoy" };
            case "taser"_hash: return { "Zeus x27", "Zeus x27" };
            case "c4"_hash: return { "C4", "C4" };
            default: break;
        }

        if (raw_name.find("knife") != std::string::npos || raw_name.find("bayonet") != std::string::npos || raw_name == "t" || raw_name == "ct") return { "Knife", "Knife" };
        return { raw_name, raw_name };
    }

    bool get_bone_position(uintptr_t pawn, uint32_t bone_id, vector3& out_pos) {
        if (!is_game_ptr(pawn)) return false;

        static uint32_t off_pGameSceneNode = 0;
        static uint32_t off_modelState = 0;

        if (!off_pGameSceneNode) off_pGameSceneNode = schema::lookup("C_BaseEntity", fnv1a::runtime_hash("m_pGameSceneNode"));
        if (!off_pGameSceneNode) return false;

        uintptr_t game_scene_node = memory::read<uintptr_t>(pawn + off_pGameSceneNode);
        if (!is_game_ptr(game_scene_node)) return false;

        if (!off_modelState) off_modelState = schema::lookup("CSkeletonInstance", fnv1a::runtime_hash("m_modelState"));
        uint32_t model_state_off = off_modelState ? off_modelState : 0x160;

        uintptr_t bone_cache = memory::read<uintptr_t>(game_scene_node + model_state_off + 0x80);
        if (!is_game_ptr(bone_cache)) return false;

        int count = memory::read<int>(game_scene_node + model_state_off + 0x8C);
        if (count <= 0 || bone_id >= static_cast<uint32_t>(count)) return false;

        out_pos = memory::read<vector3>(bone_cache + (bone_id * 0x20));
        return true;
    }

}


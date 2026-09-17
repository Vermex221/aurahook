
#include "econ_item_system.hpp"
#include "vpk_vtex.hpp"
#include "../../core/memory/memory.hpp"
#include <windows.h>
#include <algorithm>
#include <unordered_set>

namespace features::skins {

    EconItemSystem& EconItemSystem::get() {
        static EconItemSystem instance;
        return instance;
    }

    void EconItemSystem::initialize() {
        if (!items.empty()) return;

        
        items.push_back({ 1, "weapon_deagle", "Desert Eagle", ItemCategory::Weapons, 0 });
        items.push_back({ 2, "weapon_elite", "Dual Berettas", ItemCategory::Weapons, 0 });
        items.push_back({ 3, "weapon_fiveseven", "Five-SeveN", ItemCategory::Weapons, 3 });
        items.push_back({ 4, "weapon_glock", "Glock-18", ItemCategory::Weapons, 2 });
        items.push_back({ 7, "weapon_ak47", "AK-47", ItemCategory::Weapons, 2 });
        items.push_back({ 8, "weapon_aug", "AUG", ItemCategory::Weapons, 3 });
        items.push_back({ 9, "weapon_awp", "AWP", ItemCategory::Weapons, 0 });
        items.push_back({ 10, "weapon_famas", "FAMAS", ItemCategory::Weapons, 3 });
        items.push_back({ 11, "weapon_g3sg1", "G3SG1", ItemCategory::Weapons, 2 });
        items.push_back({ 13, "weapon_galilar", "Galil AR", ItemCategory::Weapons, 2 });
        items.push_back({ 14, "weapon_m249", "M249", ItemCategory::Weapons, 0 });
        items.push_back({ 16, "weapon_m4a1", "M4A4", ItemCategory::Weapons, 3 });
        items.push_back({ 17, "weapon_mac10", "MAC-10", ItemCategory::Weapons, 2 });
        items.push_back({ 19, "weapon_p90", "P90", ItemCategory::Weapons, 0 });
        items.push_back({ 23, "weapon_mp5sd", "MP5-SD", ItemCategory::Weapons, 0 });
        items.push_back({ 24, "weapon_ump45", "UMP-45", ItemCategory::Weapons, 0 });
        items.push_back({ 25, "weapon_xm1014", "XM1014", ItemCategory::Weapons, 0 });
        items.push_back({ 26, "weapon_bizon", "PP-Bizon", ItemCategory::Weapons, 0 });
        items.push_back({ 27, "weapon_mag7", "MAG-7", ItemCategory::Weapons, 3 });
        items.push_back({ 28, "weapon_negev", "Negev", ItemCategory::Weapons, 0 });
        items.push_back({ 29, "weapon_sawedoff", "Sawed-Off", ItemCategory::Weapons, 2 });
        items.push_back({ 30, "weapon_tec9", "Tec-9", ItemCategory::Weapons, 2 });
        items.push_back({ 32, "weapon_hkp2000", "P2000", ItemCategory::Weapons, 3 });
        items.push_back({ 33, "weapon_mp7", "MP7", ItemCategory::Weapons, 0 });
        items.push_back({ 34, "weapon_mp9", "MP9", ItemCategory::Weapons, 3 });
        items.push_back({ 35, "weapon_nova", "Nova", ItemCategory::Weapons, 0 });
        items.push_back({ 36, "weapon_p250", "P250", ItemCategory::Weapons, 0 });
        items.push_back({ 38, "weapon_scar20", "SCAR-20", ItemCategory::Weapons, 3 });
        items.push_back({ 39, "weapon_sg556", "SG 553", ItemCategory::Weapons, 2 });
        items.push_back({ 40, "weapon_ssg08", "SSG 08", ItemCategory::Weapons, 0 });
        items.push_back({ 60, "weapon_m4a1_silencer", "M4A1-S", ItemCategory::Weapons, 3 });
        items.push_back({ 61, "weapon_usp_silencer", "USP-S", ItemCategory::Weapons, 3 });
        items.push_back({ 63, "weapon_cz75a", "CZ75-Auto", ItemCategory::Weapons, 0 });
        items.push_back({ 64, "weapon_revolver", "R8 Revolver", ItemCategory::Weapons, 0 });

        
        items.push_back({ 500, "weapon_bayonet", "Bayonet", ItemCategory::Knives, 0 });
        items.push_back({ 503, "weapon_knife_css", "Classic Knife", ItemCategory::Knives, 0 });
        items.push_back({ 505, "weapon_knife_flip", "Flip Knife", ItemCategory::Knives, 0 });
        items.push_back({ 506, "weapon_knife_gut", "Gut Knife", ItemCategory::Knives, 0 });
        items.push_back({ 507, "weapon_knife_karambit", "Karambit", ItemCategory::Knives, 0 });
        items.push_back({ 508, "weapon_knife_m9_bayonet", "M9 Bayonet", ItemCategory::Knives, 0 });
        items.push_back({ 509, "weapon_knife_tactical", "Huntsman Knife", ItemCategory::Knives, 0 });
        items.push_back({ 512, "weapon_knife_falchion", "Falchion Knife", ItemCategory::Knives, 0 });
        items.push_back({ 514, "weapon_knife_survival_bowie", "Bowie Knife", ItemCategory::Knives, 0 });
        items.push_back({ 515, "weapon_knife_butterfly", "Butterfly Knife", ItemCategory::Knives, 0 });
        items.push_back({ 516, "weapon_knife_push", "Shadow Daggers", ItemCategory::Knives, 0 });
        items.push_back({ 517, "weapon_knife_cord", "Paracord Knife", ItemCategory::Knives, 0 });
        items.push_back({ 518, "weapon_knife_canis", "Survival Knife", ItemCategory::Knives, 0 });
        items.push_back({ 519, "weapon_knife_ursus", "Ursus Knife", ItemCategory::Knives, 0 });
        items.push_back({ 520, "weapon_knife_gypsy_jackknife", "Navaja Knife", ItemCategory::Knives, 0 });
        items.push_back({ 521, "weapon_knife_outdoor", "Nomad Knife", ItemCategory::Knives, 0 });
        items.push_back({ 522, "weapon_knife_stiletto", "Stiletto Knife", ItemCategory::Knives, 0 });
        items.push_back({ 523, "weapon_knife_widowmaker", "Talon Knife", ItemCategory::Knives, 0 });
        items.push_back({ 525, "weapon_knife_skeleton", "Skeleton Knife", ItemCategory::Knives, 0 });
        items.push_back({ 526, "weapon_knife_kukri", "Kukri Knife", ItemCategory::Knives, 0 });

        
        items.push_back({ 5027, "studded_bloodhound_gloves", "Bloodhound Gloves", ItemCategory::Gloves, 0 });
        items.push_back({ 5030, "sporty_gloves", "Sport Gloves", ItemCategory::Gloves, 0 });
        items.push_back({ 5031, "slick_gloves", "Driver Gloves", ItemCategory::Gloves, 0 });
        items.push_back({ 5032, "leather_handwraps", "Hand Wraps", ItemCategory::Gloves, 0 });
        items.push_back({ 5033, "motorcycle_gloves", "Moto Gloves", ItemCategory::Gloves, 0 });
        items.push_back({ 5034, "specialist_gloves", "Specialist Gloves", ItemCategory::Gloves, 0 });
        items.push_back({ 5035, "studded_hydra_gloves", "Hydra Gloves", ItemCategory::Gloves, 0 });
        items.push_back({ 5036, "studded_brokenfang_gloves", "Broken Fang Gloves", ItemCategory::Gloves, 0 });

        
        items.push_back({ 4619, "customplayer_ctm_diver_varianta", "Cmdr. Davida 'Goggles' Fernandez", ItemCategory::Agents, 3 });
        items.push_back({ 4680, "customplayer_ctm_fbi_variantf", "Special Agent Ava", ItemCategory::Agents, 3 });
        items.push_back({ 4699, "customplayer_ctm_gendarmerie_varianta", "Sous-Lieutenant Medic", ItemCategory::Agents, 3 });
        items.push_back({ 4705, "customplayer_ctm_sas_variantg", "D Squadron Officer", ItemCategory::Agents, 3 });
        items.push_back({ 4711, "customplayer_ctm_st6_variantk", "Lt. Commander Rickawede", ItemCategory::Agents, 3 });
        items.push_back({ 4712, "customplayer_ctm_st6_variantm", "'Blueberries' Buckshot", ItemCategory::Agents, 3 });
        items.push_back({ 4713, "customplayer_ctm_st6_varianti", "Two Times McCoy", ItemCategory::Agents, 3 });
        items.push_back({ 4715, "customplayer_ctm_swat_variante", "Cmdr. Mae 'Dead Cold' Jamison", ItemCategory::Agents, 3 });
        items.push_back({ 4726, "customplayer_tm_balkan_variantk", "Dragomir | Sabre", ItemCategory::Agents, 2 });
        items.push_back({ 4732, "customplayer_tm_jumpsuit_varianta", "Number K | The Professionals", ItemCategory::Agents, 2 });
        items.push_back({ 4733, "customplayer_tm_jumpsuit_variantb", "Sir Bloody Miami Darryl", ItemCategory::Agents, 2 });
        items.push_back({ 4734, "customplayer_tm_jumpsuit_variantc", "Sir Bloody Silent Darryl", ItemCategory::Agents, 2 });
        items.push_back({ 4735, "customplayer_tm_jumpsuit_variantc", "Sir Bloody Skullhead Darryl", ItemCategory::Agents, 2 });
        items.push_back({ 4749, "customplayer_tm_professional_varf", "Getaway Sally", ItemCategory::Agents, 2 });
        items.push_back({ 4752, "customplayer_tm_professional_varianti", "Safecracker Voltzmann", ItemCategory::Agents, 2 });

        for (size_t i = 0; i < items.size(); ++i) {
            item_map[items[i].def_index] = i;
        }

        
        paint_kits.push_back({ 0, "default", "Default", 1 });
        paint_kits.push_back({ 38, "so_fade", "Fade", 6 });
        paint_kits.push_back({ 44, "aq_oiled", "Case Hardened", 5 });
        paint_kits.push_back({ 12, "hy_webs", "Crimson Web", 5 });
        paint_kits.push_back({ 59, "am_slaughter", "Slaughter", 5 });
        paint_kits.push_back({ 409, "am_tiger_orange", "Tiger Tooth", 5 });
        paint_kits.push_back({ 413, "am_marble_fade", "Marble Fade", 5 });
        paint_kits.push_back({ 415, "am_ruby_marble", "Doppler (Ruby)", 6 });
        paint_kits.push_back({ 416, "am_sapphire_marble", "Doppler (Sapphire)", 6 });
        paint_kits.push_back({ 417, "am_blackpearl_marble", "Doppler (Black Pearl)", 6 });
        paint_kits.push_back({ 418, "am_doppler_phase1", "Doppler (Phase 1)", 5 });
        paint_kits.push_back({ 419, "am_doppler_phase2", "Doppler (Phase 2)", 5 });
        paint_kits.push_back({ 420, "am_doppler_phase3", "Doppler (Phase 3)", 5 });
        paint_kits.push_back({ 421, "am_doppler_phase4", "Doppler (Phase 4)", 5 });
        paint_kits.push_back({ 568, "am_emerald_marble", "Gamma Doppler (Emerald)", 6 });
        paint_kits.push_back({ 569, "am_gamma_doppler_phase1", "Gamma Doppler (Phase 1)", 5 });
        paint_kits.push_back({ 570, "am_gamma_doppler_phase2", "Gamma Doppler (Phase 2)", 5 });
        paint_kits.push_back({ 571, "am_gamma_doppler_phase3", "Gamma Doppler (Phase 3)", 5 });
        paint_kits.push_back({ 572, "am_gamma_doppler_phase4", "Gamma Doppler (Phase 4)", 5 });
        paint_kits.push_back({ 561, "cu_lore", "Lore", 5 });
        paint_kits.push_back({ 573, "gs_autotronic", "Autotronic", 5 });
        paint_kits.push_back({ 143, "so_purple", "Ultraviolet", 4 });
        paint_kits.push_back({ 175, "am_ddpat_green", "Forest DDPAT", 2 });
        paint_kits.push_back({ 98, "so_night", "Night", 3 });
        paint_kits.push_back({ 344, "cu_dragon_lore", "Dragon Lore", 6 });
        paint_kits.push_back({ 448, "cu_medusa", "Medusa", 6 });
        paint_kits.push_back({ 756, "cu_gungnir", "Gungnir", 6 });
        paint_kits.push_back({ 309, "cu_howl", "Howl", 7 });
        paint_kits.push_back({ 180, "cu_fire_serpent", "Fire Serpent", 5 });
        paint_kits.push_back({ 724, "cu_wild_lotus", "Wild Lotus", 6 });
        paint_kits.push_back({ 1119, "cu_gold_arabesque", "Gold Arabesque", 6 });
        paint_kits.push_back({ 302, "cu_ak47_vulcan", "Vulcan", 5 });
        paint_kits.push_back({ 255, "cu_m4a4_asiimov", "Asiimov", 5 });
        paint_kits.push_back({ 282, "cu_ak47_redline", "Redline", 4 });
        paint_kits.push_back({ 430, "cu_hyper_beast", "Hyper Beast", 5 });
        paint_kits.push_back({ 984, "cu_printstream", "Printstream", 5 });
        paint_kits.push_back({ 488, "cu_neon_rider", "Neon Rider", 5 });
        paint_kits.push_back({ 102, "so_whiteout", "Whiteout", 3 });
        paint_kits.push_back({ 185, "so_red", "Hot Rod", 4 });
        paint_kits.push_back({ 638, "cu_ak47_bloodsport", "Bloodsport", 5 });
        paint_kits.push_back({ 707, "cu_ak47_neon_revolution", "Neon Revolution", 5 });
        paint_kits.push_back({ 1035, "cu_ak47_slate", "Slate", 3 });
        paint_kits.push_back({ 1144, "cu_ak47_ice_coaled", "Ice Coaled", 4 });
        paint_kits.push_back({ 1221, "cu_ak47_head_shot", "Head Shot", 6 });
        paint_kits.push_back({ 1252, "cu_ak47_inheritance", "Inheritance", 6 });
        paint_kits.push_back({ 917, "cu_awp_wildfire", "Wildfire", 5 });
        paint_kits.push_back({ 803, "cu_awp_neonoir", "Neo-Noir (AWP)", 5 });
        paint_kits.push_back({ 888, "cu_awp_containment", "Containment Breach", 5 });
        paint_kits.push_back({ 1145, "cu_awp_chromatic", "Chromatic Aberration", 5 });
        paint_kits.push_back({ 1222, "cu_awp_duality", "Duality", 4 });
        paint_kits.push_back({ 695, "cu_m4a4_neo_noir", "Neo-Noir (M4A4)", 5 });
        paint_kits.push_back({ 844, "cu_m4a4_emperor", "The Emperor", 5 });
        paint_kits.push_back({ 1041, "cu_m4a4_in_living_color", "In Living Color", 5 });
        paint_kits.push_back({ 1228, "cu_m4a4_temukau", "Temukau", 5 });
        paint_kits.push_back({ 632, "cu_m4a4_buzz_kill", "Buzz Kill", 4 });
        paint_kits.push_back({ 400, "cu_m4a4_ancestral", "Dragon King", 4 });
        paint_kits.push_back({ 449, "cu_deagle_sunset_storm", "Sunset Storm 壱", 4 });
        paint_kits.push_back({ 711, "cu_deagle_code_red", "Code Red", 5 });
        paint_kits.push_back({ 353, "cu_deagle_conspiracy", "Conspiracy", 4 });
        paint_kits.push_back({ 527, "cu_deagle_kumicho_dragon", "Kumicho Dragon", 4 });
        paint_kits.push_back({ 37, "so_blaze", "Blaze", 5 });
        paint_kits.push_back({ 1018, "cu_deagle_fennec_fox", "Fennec Fox", 5 });
        paint_kits.push_back({ 1090, "cu_deagle_ocean_drive", "Ocean Drive", 5 });
        paint_kits.push_back({ 757, "cu_deagle_jormungandr", "Emerald Jörmungandr", 4 });
        paint_kits.push_back({ 257, "cu_m4a1_hyper_beast", "Hyper Beast (M4A1-S)", 5 });
        paint_kits.push_back({ 440, "cu_m4a1_cyrex", "Cyrex", 5 });
        paint_kits.push_back({ 945, "cu_m4a1_player_two", "Player Two", 5 });
        paint_kits.push_back({ 1004, "cu_m4a1_jungle", "Welcome to the Jungle", 6 });
        paint_kits.push_back({ 1017, "cu_m4a1_blue_phosphor", "Blue Phosphor", 5 });
        paint_kits.push_back({ 548, "cu_m4a1_chantico", "Chantico's Fire", 5 });
        paint_kits.push_back({ 644, "cu_m4a1_decimator", "Decimator", 4 });
        paint_kits.push_back({ 714, "cu_m4a1_nightmare", "Nightmare", 4 });
        paint_kits.push_back({ 497, "cu_m4a1_golden_coil", "Golden Coil", 5 });
        paint_kits.push_back({ 504, "cu_usp_kill_confirmed", "Kill Confirmed", 5 });
        paint_kits.push_back({ 653, "cu_usp_neo_noir", "Neo-Noir (USP-S)", 5 });
        paint_kits.push_back({ 705, "cu_usp_cortex", "Cortex", 4 });
        paint_kits.push_back({ 1040, "cu_usp_traitor", "The Traitor", 5 });
        paint_kits.push_back({ 313, "cu_usp_orion", "Orion", 4 });
        paint_kits.push_back({ 1253, "cu_usp_jawbreaker", "Jawbreaker", 4 });
        paint_kits.push_back({ 357, "cu_glock_water_elemental", "Water Elemental", 4 });
        paint_kits.push_back({ 852, "cu_glock_bullet_queen", "Bullet Queen", 5 });
        paint_kits.push_back({ 963, "cu_glock_vogue", "Vogue", 4 });
        paint_kits.push_back({ 1100, "cu_glock_snack_attack", "Snack Attack", 4 });
        paint_kits.push_back({ 988, "cu_glock_neo_noir", "Neo-Noir (Glock)", 4 });
        paint_kits.push_back({ 586, "cu_glock_wasteland_rebel", "Wasteland Rebel", 5 });
        paint_kits.push_back({ 678, "cu_p250_cybercroc", "See Ya Later", 5 });
        paint_kits.push_back({ 1028, "cu_p250_apep", "Apep's Curse", 4 });
        paint_kits.push_back({ 1148, "cu_p250_visions", "Visions", 4 });
        paint_kits.push_back({ 734, "cu_mp9_wild_lily", "Wild Lily", 5 });
        paint_kits.push_back({ 1095, "cu_mp9_mount_fuji", "Mount Fuji", 4 });
        paint_kits.push_back({ 1140, "cu_mp9_starlight", "Starlight Protector", 5 });
        paint_kits.push_back({ 1050, "cu_mp9_food_chain", "Food Chain", 4 });
        paint_kits.push_back({ 897, "cu_mac10_stalker", "Stalker", 5 });
        paint_kits.push_back({ 933, "cu_mac10_disco_tech", "Disco Tech", 4 });
        paint_kits.push_back({ 433, "cu_mac10_neon_rider", "Neon Rider (MAC-10)", 4 });
        paint_kits.push_back({ 222, "cu_ssg08_blood_in_water", "Blood in the Water", 5 });
        paint_kits.push_back({ 624, "cu_ssg08_dragonfire", "Dragonfire", 4 });
        paint_kits.push_back({ 1099, "cu_ssg08_turbo_peek", "Turbo Peek", 4 });
        paint_kits.push_back({ 398, "cu_galil_chatterbox", "Chatterbox", 5 });
        paint_kits.push_back({ 661, "cu_galil_sugar_rush", "Sugar Rush", 4 });
        paint_kits.push_back({ 919, "cu_famas_commemoration", "Commemoration", 5 });
        paint_kits.push_back({ 604, "cu_famas_roll_cage", "Roll Cage", 5 });
        paint_kits.push_back({ 750, "cu_sg553_integrale", "Integrale", 4 });
        paint_kits.push_back({ 899, "cu_sg553_colony_iv", "Colony IV", 4 });
        paint_kits.push_back({ 455, "cu_aug_akihabara", "Akihabara Accept", 5 });
        paint_kits.push_back({ 758, "cu_aug_flame_jormungandr", "Flame Jörmungandr", 4 });
        paint_kits.push_back({ 481, "cu_mp7_nemesis", "Nemesis", 4 });
        paint_kits.push_back({ 1147, "cu_mp7_abyssal", "Abyssal Apparition", 4 });
        paint_kits.push_back({ 556, "cu_ump45_primal_saber", "Primal Saber", 4 });
        paint_kits.push_back({ 802, "cu_ump45_momentum", "Momentum", 4 });
        paint_kits.push_back({ 156, "cu_p90_kitty", "Death by Kitty", 4 });
        paint_kits.push_back({ 1019, "cu_p90_run_and_hide", "Run and Hide", 4 });
        paint_kits.push_back({ 270, "cu_cz75_victoria", "Victoria", 4 });
        paint_kits.push_back({ 453, "cu_cz75_emerald", "Emerald", 3 });
        paint_kits.push_back({ 658, "cu_dual_berettas_cobra", "Cobra Strike", 4 });
        paint_kits.push_back({ 1146, "cu_dual_berettas_flora", "Flora Carnivora", 4 });
        paint_kits.push_back({ 660, "cu_five_seven_hyper_beast", "Hyper Beast (Five-SeveN)", 5 });
        paint_kits.push_back({ 427, "cu_five_seven_banana", "Monkey Business", 4 });
        paint_kits.push_back({ 845, "cu_five_seven_angry_mob", "Angry Mob", 4 });
        paint_kits.push_back({ 1149, "cu_revolver_crazy_8", "Crazy 8", 4 });
        paint_kits.push_back({ 1056, "cu_revolver_banana", "Banana Cannon", 4 });
        paint_kits.push_back({ 10048, "cu_gloves_vice", "Vice (Sport Gloves)", 6 });
        paint_kits.push_back({ 10037, "cu_gloves_pandora", "Pandora's Box (Sport Gloves)", 6 });
        paint_kits.push_back({ 10044, "cu_gloves_black_tie", "Black Tie (Driver Gloves)", 5 });
        paint_kits.push_back({ 10045, "cu_gloves_amphibious", "Amphibious (Sport Gloves)", 6 });
        paint_kits.push_back({ 10041, "cu_gloves_king_snake", "King Snake (Driver Gloves)", 6 });
        paint_kits.push_back({ 10047, "cu_gloves_snow_leopard", "Snow Leopard (Driver Gloves)", 6 });

        for (size_t i = 0; i < paint_kits.size(); ++i) {
            paint_kit_map[paint_kits[i].id] = i;
        }

        parse_from_game();
    }

    bool EconItemSystem::parse_from_game() {
        uintptr_t fn = memory::pattern_scan("client.dll", "48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 81");
        if (!fn) return false;

        uintptr_t slot = memory::resolve_rip(fn + 4, 3, 7);
        if (!slot) return false;

        uintptr_t system = memory::read<uintptr_t>(slot);
        if (!system) {
            auto get_system = reinterpret_cast<uintptr_t(__fastcall*)()>(fn);
            if (get_system) system = get_system();
        }
        if (!system || system < 0x10000) return false;

        uintptr_t schema = memory::read<uintptr_t>(system + 0x8);
        if (!schema || schema < 0x10000) return false;

        int paint_count = memory::read<int>(schema + 0x2F0);
        uintptr_t paint_nodes = memory::read<uintptr_t>(schema + 0x2F8);

        if (paint_count <= 0 || paint_count > 10000 || !paint_nodes || paint_nodes < 0x10000) {
            return false;
        }

        typedef void* (*CreateInterfaceFn)(const char*, int*);
        HMODULE h_loc = GetModuleHandleA("localize.dll");
        void* localize = nullptr;
        if (h_loc) {
            auto ci = (CreateInterfaceFn)GetProcAddress(h_loc, "CreateInterface");
            if (ci) localize = ci("Localize_001", nullptr);
        }

        std::vector<PaintKit> loaded_kits;
        loaded_kits.reserve(paint_count + 1);
        loaded_kits.push_back({ 0, "default", "Default", 1 });

        std::unordered_map<int, size_t> new_map;
        new_map[0] = 0;

        for (int i = 0; i < paint_count; ++i) {
            uintptr_t node_base = paint_nodes + static_cast<uintptr_t>(32 * i);
            int key = memory::read<int>(node_base + 16);
            uintptr_t pk_ptr = memory::read<uintptr_t>(node_base + 24);
            if (!pk_ptr || pk_ptr < 0x10000) continue;

            int id = memory::read<int>(pk_ptr + 0x00);
            if (id <= 0) id = key;
            if (id <= 0 || new_map.contains(id)) continue;

            int rarity = memory::read<int>(pk_ptr + 0x44) & 0xFF;

            uintptr_t name_ptr = memory::read<uintptr_t>(pk_ptr + 0x08);
            std::string name = name_ptr ? memory::read_string(name_ptr) : "";

            uintptr_t name_tok_ptr = memory::read<uintptr_t>(pk_ptr + 0x18);
            std::string name_token = name_tok_ptr ? memory::read_string(name_tok_ptr) : "";

            std::string display_name;
            if (localize && !name_token.empty()) {
                const char* loc = memory::call_vfunc<const char*>(localize, 17, name_token.c_str());
                if (loc && *loc && strcmp(loc, name_token.c_str()) != 0) {
                    display_name = loc;
                }
            }
            if (display_name.empty()) {
                display_name = name;
            }

            new_map[id] = loaded_kits.size();
            loaded_kits.push_back({ id, std::move(name), std::move(display_name), static_cast<uint8_t>(rarity) });
        }

        if (loaded_kits.size() > 50) {
            paint_kits = std::move(loaded_kits);
            paint_kit_map = std::move(new_map);
            skin_index_built = false;
            build_skin_index();
            return true;
        }

        return false;
    }

    static bool glove_kit_belongs(const char* simple, const std::string& pk_name) {
        if (!simple || !*simple || pk_name.empty()) return false;
        if (strcmp(simple, "studded_hydra_gloves") == 0)
            return pk_name.rfind("bloodhound_hydra_", 0) == 0;
        if (strcmp(simple, "studded_bloodhound_gloves") == 0)
            return pk_name.rfind("bloodhound_", 0) == 0 && pk_name.rfind("bloodhound_hydra_", 0) != 0;
        if (strcmp(simple, "studded_brokenfang_gloves") == 0)
            return pk_name.rfind("operation10_", 0) == 0;
        if (strcmp(simple, "sporty_gloves") == 0)
            return pk_name.rfind("sporty_", 0) == 0 || pk_name.rfind("glove_sport_", 0) == 0;
        if (strcmp(simple, "slick_gloves") == 0)
            return pk_name.rfind("slick_", 0) == 0 || pk_name.rfind("glove_driver_", 0) == 0;
        if (strcmp(simple, "leather_handwraps") == 0)
            return pk_name.rfind("handwrap_", 0) == 0;
        if (strcmp(simple, "motorcycle_gloves") == 0)
            return pk_name.rfind("motorcycle_", 0) == 0;
        if (strcmp(simple, "specialist_gloves") == 0)
            return pk_name.rfind("specialist_", 0) == 0 || pk_name.rfind("glove_specialist_", 0) == 0;
        return false;
    }

    void EconItemSystem::build_skin_index() {
        if (!vpk_vtex::is_vpk_loaded()) return;

        const auto& vpk = vpk_vtex::get_vpk_index();
        if (vpk.empty()) return;

        item_skins_map.clear();

        std::unordered_map<std::string, int> pk_by_name;
        pk_by_name.reserve(paint_kits.size() * 2);
        for (const auto& pk : paint_kits) {
            if (!pk.name.empty()) {
                pk_by_name[pk.name] = pk.id;
            }
        }

        std::unordered_map<std::string, int16_t> def_by_name;
        def_by_name.reserve(items.size() * 4);
        auto add_def = [&](const std::string& name, int16_t def) {
            if (name.empty()) return;
            def_by_name[name] = def;
            if (name.rfind("weapon_", 0) == 0 && name.size() > 7) {
                def_by_name[name.substr(7)] = def;
            }
        };

        for (const auto& item : items) {
            if (item.category != ItemCategory::Weapons && item.category != ItemCategory::Knives && item.category != ItemCategory::Gloves)
                continue;
            add_def(item.name, item.def_index);
            if (item.name == "weapon_m4a1") {
                add_def("m4a4", item.def_index);
                add_def("weapon_m4a4", item.def_index);
            } else if (item.name == "weapon_m4a1_silencer") {
                add_def("m4a1_s", item.def_index);
                add_def("m4a1s", item.def_index);
                add_def("weapon_m4a1s", item.def_index);
                add_def("weapon_m4a1_s", item.def_index);
            } else if (item.name == "weapon_usp_silencer") {
                add_def("usp_s", item.def_index);
                add_def("usps", item.def_index);
                add_def("weapon_usps", item.def_index);
                add_def("weapon_usp_s", item.def_index);
                add_def("usp", item.def_index);
            } else if (item.name == "weapon_deagle") {
                add_def("deag", item.def_index);
            } else if (item.name == "weapon_galilar") {
                add_def("galil", item.def_index);
                add_def("weapon_galil", item.def_index);
            } else if (item.name == "weapon_sg556") {
                add_def("sg553", item.def_index);
                add_def("weapon_sg553", item.def_index);
            } else if (item.name == "weapon_hkp2000") {
                add_def("p2000", item.def_index);
                add_def("weapon_p2000", item.def_index);
            } else if (item.name == "weapon_cz75a") {
                add_def("cz75", item.def_index);
                add_def("weapon_cz75", item.def_index);
            } else if (item.name == "weapon_revolver") {
                add_def("r8", item.def_index);
                add_def("revolver_r8", item.def_index);
                add_def("weapon_r8", item.def_index);
            } else if (item.name == "weapon_bizon") {
                add_def("mp_bizon", item.def_index);
                add_def("weapon_mp_bizon", item.def_index);
            } else if (item.name == "weapon_elite") {
                add_def("dual_berettas", item.def_index);
                add_def("weapon_dual_berettas", item.def_index);
            } else if (item.name == "weapon_ssg08") {
                add_def("scout", item.def_index);
                add_def("weapon_scout", item.def_index);
            }
        }

        constexpr std::string_view prefix = "econ/default_generated/";
        constexpr std::string_view suffix = "_light_png";

        for (const auto& [path, _] : vpk) {
            if (path.rfind("econ/default_generated/", 0) != 0 || !path.ends_with("_light_png"))
                continue;

            std::string stem = path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
            int16_t matched_def = -1;
            std::string pk_part;

            for (size_t i = stem.find('_', 1); i != std::string::npos; i = stem.find('_', i + 1)) {
                std::string cand = stem.substr(0, i);
                auto it = def_by_name.find(cand);
                if (it != def_by_name.end()) {
                    matched_def = it->second;
                    pk_part = stem.substr(i + 1);
                }
            }

            if (matched_def != -1 && !pk_part.empty()) {
                auto it = pk_by_name.find(pk_part);
                if (it == pk_by_name.end()) {
                    size_t last_us = pk_part.rfind('_');
                    if (last_us != std::string::npos) {
                        it = pk_by_name.find(pk_part.substr(0, last_us));
                    }
                }
                if (it != pk_by_name.end()) {
                    item_skins_map[matched_def].push_back(it->second);
                }
            }
        }

        for (const auto& item : items) {
            if (item.category == ItemCategory::Gloves) {
                auto& list = item_skins_map[item.def_index];
                for (const auto& pk : paint_kits) {
                    if (glove_kit_belongs(item.name.c_str(), pk.name)) {
                        list.push_back(pk.id);
                    }
                }
            } else if (item.category == ItemCategory::Knives) {
                static const std::vector<int> knife_popular = {
                    38, 44, 12, 59, 409, 413, 415, 416, 417, 418, 419, 420, 421,
                    568, 569, 570, 571, 572, 561, 573, 143, 175, 98
                };
                auto& list = item_skins_map[item.def_index];
                for (int kid : knife_popular) {
                    if (find_paint_kit(kid)) {
                        list.push_back(kid);
                    }
                }
            }
        }

        for (auto& [def, list] : item_skins_map) {
            std::unordered_set<int> seen;
            std::vector<int> unique_list;
            unique_list.reserve(list.size());
            for (int id : list) {
                if (id > 0 && seen.insert(id).second) {
                    unique_list.push_back(id);
                }
            }
            list = std::move(unique_list);
            std::sort(list.begin(), list.end(), [this](int a, int b) {
                const auto* pka = find_paint_kit(a);
                const auto* pkb = find_paint_kit(b);
                const std::string& na = pka ? pka->display_name : "";
                const std::string& nb = pkb ? pkb->display_name : "";
                return na < nb;
            });
        }

        skin_index_built = true;
    }

    std::vector<const PaintKit*> EconItemSystem::get_paint_kits_for_item(int16_t def_index) {
        if (paint_kits.size() <= 150) {
            parse_from_game();
        }

        if (!skin_index_built && vpk_vtex::is_vpk_loaded()) {
            build_skin_index();
        }

        std::vector<const PaintKit*> result;
        const auto* item = find_item(def_index);
        if (!item) return result;

        const auto* def_pk = find_paint_kit(0);
        if (def_pk) {
            result.push_back(def_pk);
        }

        auto it = item_skins_map.find(def_index);
        if (it != item_skins_map.end() && !it->second.empty()) {
            for (int pkid : it->second) {
                if (pkid == 0) continue;
                const auto* pk = find_paint_kit(pkid);
                if (pk) result.push_back(pk);
            }
            return result;
        }

        if (item->category == ItemCategory::Knives) {
            static const std::vector<int> knife_fallback = {
                38, 44, 12, 59, 409, 413, 415, 416, 417, 418, 419, 420, 421,
                568, 569, 570, 571, 572, 561, 573, 143, 175, 98
            };
            for (int kid : knife_fallback) {
                const auto* pk = find_paint_kit(kid);
                if (pk) result.push_back(pk);
            }
        } else if (item->category == ItemCategory::Gloves) {
            for (const auto& pk : paint_kits) {
                if (pk.id > 0 && glove_kit_belongs(item->name.c_str(), pk.name)) {
                    result.push_back(&pk);
                }
            }
        } else if (item->category == ItemCategory::Weapons) {
            std::string stem = item->name;
            if (stem.rfind("weapon_", 0) == 0) stem.erase(0, 7);
            for (const auto& pk : paint_kits) {
                if (pk.id > 0 && pk.id < 10000 && pk.name.find(stem) != std::string::npos) {
                    result.push_back(&pk);
                }
            }
        }
        return result;
    }

    const ItemDef* EconItemSystem::find_item(int16_t def_index) const {
        auto it = item_map.find(def_index);
        if (it != item_map.end()) {
            return &items[it->second];
        }
        return nullptr;
    }

    const PaintKit* EconItemSystem::find_paint_kit(int id) const {
        auto it = paint_kit_map.find(id);
        if (it != paint_kit_map.end()) {
            return &paint_kits[it->second];
        }
        return nullptr;
    }

    std::vector<const ItemDef*> EconItemSystem::get_items_by_category(ItemCategory cat) const {
        std::vector<const ItemDef*> result;
        for (const auto& item : items) {
            if (item.category == cat) {
                result.push_back(&item);
            }
        }
        return result;
    }

}


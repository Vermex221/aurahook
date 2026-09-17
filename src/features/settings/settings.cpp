#include "settings.hpp"
#include "../legit/legit.hpp"
#include "../rage/rage.hpp"
#include "../visuals/visuals.hpp"
#include "../misc/misc.hpp"
#include "../skins/skins.hpp"
#include "../../widgets/custom_widgets.hpp"
#include "../../gui/gui/gui.hpp"
#include "../../gui/theme/theme.hpp"

#include <windows.h>
#include <algorithm>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace features::settings {

    static void write_kv(std::ofstream& out, const std::string& k, const std::string& v) {
        out << k << "=" << v << "\n";
    }

    static void write_bool(std::ofstream& out, const std::string& k, bool v) {
        out << k << "=" << (v ? "1" : "0") << "\n";
    }

    static void write_int(std::ofstream& out, const std::string& k, int v) {
        out << k << "=" << v << "\n";
    }

    static void write_float(std::ofstream& out, const std::string& k, float v) {
        out << k << "=" << v << "\n";
    }

    static void write_color(std::ofstream& out, const std::string& k, const float c[4]) {
        out << k << "=" << c[0] << "," << c[1] << "," << c[2] << "," << c[3] << "\n";
    }

    static void parse_color(const std::string& s, float c[4]) {
        std::stringstream ss(s);
        std::string item;
        int i = 0;
        while (std::getline(ss, item, ',') && i < 4) {
            c[i++] = std::stof(item);
        }
    }

    void init_configs() {
        CreateDirectoryA("configs", NULL);

        configs.clear();
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA("configs\\*.cfg", &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string fname = fd.cFileName;
                if (fname.size() > 4 && fname.substr(fname.size() - 4) == ".cfg") {
                    configs.push_back(fname.substr(0, fname.size() - 4));
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }

        if (configs.empty()) {
            configs = { "Default" };
            save_config("Default");
        }
    }

    void save_config(const std::string& name) {
        if (name.empty()) return;
        CreateDirectoryA("configs", NULL);

        std::string path = "configs\\" + name + ".cfg";
        std::ofstream out(path);
        if (!out.is_open()) return;

        write_color(out, "accent_color", cfg.accent_color);
        write_bool(out, "menu_bind", cfg.menu_bind);
        write_int(out, "menu_key", cfg.menu_key);
        write_int(out, "menu_key_mode", cfg.menu_key_mode);
        write_int(out, "selected_font", cfg.selected_font);
        write_bool(out, "custom_menu_name", cfg.custom_menu_name);
        write_kv(out, "menu_name", cfg.menu_name);

        write_bool(out, "watermark", cfg.watermark);
        write_bool(out, "keybind_list", cfg.keybind_list);
        write_bool(out, "spectator_list", misc::cfg.spectator_list);
        write_bool(out, "bomb_info", misc::cfg.bomb_info);

        auto save_esp = [&](const std::string& prefix, const visuals::EspTargetSettings& esp) {
            write_bool(out, prefix + "_box", esp.box);
            write_color(out, prefix + "_box_color", esp.box_color);

            write_bool(out, prefix + "_health", esp.health);
            write_bool(out, prefix + "_health_gradient", esp.health_gradient);
            write_color(out, prefix + "_health_gradient_color", esp.health_gradient_color);
            write_bool(out, prefix + "_health_outline", esp.health_outline);
            write_color(out, prefix + "_health_outline_color", esp.health_outline_color);
            write_bool(out, prefix + "_health_background", esp.health_background);
            write_color(out, prefix + "_health_bg_color", esp.health_bg_color);
        };

        auto save_chams = [&](const std::string& prefix, const visuals::ChamsTargetSettings& chams) {
            write_bool(out, prefix + "_chams_enabled", chams.enabled);
            write_int(out, prefix + "_chams_material", chams.material);
            write_color(out, prefix + "_chams_visible_color", chams.visible_color);
            write_bool(out, prefix + "_chams_occluded", chams.occluded);
            write_color(out, prefix + "_chams_occluded_color", chams.occluded_color);
        };

        save_esp("enemy", visuals::cfg.enemy);
        save_esp("team", visuals::cfg.team);
        save_esp("local", visuals::cfg.local);

        save_chams("enemy", visuals::cfg.chams_enemy);
        save_chams("team", visuals::cfg.chams_team);
        save_chams("local", visuals::cfg.chams_local);
        save_chams("arms", visuals::cfg.chams_arms);
        save_chams("weapon", visuals::cfg.chams_weapon);

        save_chams("ragdoll_enemy", visuals::cfg.chams_ragdoll_enemy);
        save_chams("ragdoll_team", visuals::cfg.chams_ragdoll_team);
        save_chams("ragdoll_local", visuals::cfg.chams_ragdoll_local);

        static const std::string weapon_prefixes[6] = { "pistol", "smg", "shotgun", "rifle", "sniper", "lmg" };
        for (int i = 0; i < 6; ++i) {
            const auto& w = legit::cfg.weapons[i];
            std::string prefix = "legit_" + weapon_prefixes[i] + "_";
            write_bool(out, prefix + "enabled", w.enabled);
            write_int(out, prefix + "key", w.key);
            write_int(out, prefix + "key_mode", w.key_mode);
            write_int(out, prefix + "target_part", w.target_part);
            write_float(out, prefix + "fov", w.fov);
            write_float(out, prefix + "smooth", w.smooth);
            write_bool(out, prefix + "draw_fov", w.draw_fov);
            write_color(out, prefix + "fov_color", w.fov_color);

            write_bool(out, prefix + "trigger_enabled", w.trigger_enabled);
            write_int(out, prefix + "trigger_key", w.trigger_key);
            write_int(out, prefix + "trigger_key_mode", w.trigger_key_mode);
            write_bool(out, prefix + "trigger_filter_enabled", w.trigger_filter_enabled);
            write_int(out, prefix + "trigger_target_part", w.trigger_target_part);
            write_int(out, prefix + "trigger_delay", w.trigger_delay);
            write_bool(out, prefix + "trigger_seeded_delay", w.trigger_seeded_delay);
        }

        write_bool(out, "skins_enabled", skins::cfg.enabled);

        write_int(out, "skins_knife_def", skins::cfg.equipped_knife_def);
        write_int(out, "skins_knife_paint_kit", skins::cfg.equipped_knife_skin.paint_kit_id);
        write_float(out, "skins_knife_wear", skins::cfg.equipped_knife_skin.wear);
        write_int(out, "skins_knife_seed", skins::cfg.equipped_knife_skin.seed);
        write_bool(out, "skins_knife_stattrak", skins::cfg.equipped_knife_skin.stattrak);
        write_int(out, "skins_knife_stattrak_count", skins::cfg.equipped_knife_skin.stattrak_count);

        write_int(out, "skins_glove_def", skins::cfg.equipped_glove_def);
        write_int(out, "skins_glove_paint_kit", skins::cfg.equipped_glove_skin.paint_kit_id);
        write_float(out, "skins_glove_wear", skins::cfg.equipped_glove_skin.wear);
        write_int(out, "skins_glove_seed", skins::cfg.equipped_glove_skin.seed);
        write_bool(out, "skins_glove_stattrak", skins::cfg.equipped_glove_skin.stattrak);
        write_int(out, "skins_glove_stattrak_count", skins::cfg.equipped_glove_skin.stattrak_count);

        write_int(out, "skins_agent_ct", skins::cfg.equipped_agent_ct);
        write_int(out, "skins_agent_t", skins::cfg.equipped_agent_t);

        write_int(out, "skins_weapon_count", static_cast<int>(skins::cfg.weapon_skins.size()));
        int w_idx = 0;
        for (const auto& [def_idx, skin] : skins::cfg.weapon_skins) {
            std::string prefix = "skins_weapon_" + std::to_string(w_idx) + "_";
            write_int(out, prefix + "def", def_idx);
            write_int(out, prefix + "paint_kit", skin.paint_kit_id);
            write_float(out, prefix + "wear", skin.wear);
            write_int(out, prefix + "seed", skin.seed);
            write_bool(out, prefix + "stattrak", skin.stattrak);
            write_int(out, prefix + "stattrak_count", skin.stattrak_count);
            w_idx++;
        }

        out.close();

        if (std::find(configs.begin(), configs.end(), name) == configs.end()) {
            configs.push_back(name);
        }
    }

    void load_config(const std::string& name) {
        if (name.empty()) return;
        std::string path = "configs\\" + name + ".cfg";
        std::ifstream in(path);
        if (!in.is_open()) return;

        std::unordered_map<std::string, std::string> kv;
        std::string line;
        while (std::getline(in, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                kv[line.substr(0, eq)] = line.substr(eq + 1);
            }
        }
        in.close();

        auto get_b = [&](const std::string& k, bool def) {
            auto it = kv.find(k);
            return (it != kv.end()) ? (it->second == "1") : def;
        };
        auto get_i = [&](const std::string& k, int def) {
            auto it = kv.find(k);
            return (it != kv.end()) ? std::stoi(it->second) : def;
        };
        auto get_f = [&](const std::string& k, float def) {
            auto it = kv.find(k);
            return (it != kv.end()) ? std::stof(it->second) : def;
        };
        auto get_col = [&](const std::string& k, float c[4]) {
            auto it = kv.find(k);
            if (it != kv.end()) parse_color(it->second, c);
        };
        auto get_s = [&](const std::string& k, char* buf, size_t sz) {
            auto it = kv.find(k);
            if (it != kv.end()) snprintf(buf, sz, "%s", it->second.c_str());
        };

        get_col("accent_color", cfg.accent_color);
        gui::theme::accent_color = ImVec4(cfg.accent_color[0], cfg.accent_color[1], cfg.accent_color[2], cfg.accent_color[3]);
        gui::theme::apply();

        cfg.menu_bind = get_b("menu_bind", cfg.menu_bind);
        cfg.menu_key = get_i("menu_key", cfg.menu_key);
        cfg.menu_key_mode = get_i("menu_key_mode", cfg.menu_key_mode);
        cfg.selected_font = get_i("selected_font", cfg.selected_font);
        cfg.custom_menu_name = get_b("custom_menu_name", cfg.custom_menu_name);
        get_s("menu_name", cfg.menu_name, sizeof(cfg.menu_name));

        cfg.watermark = get_b("watermark", cfg.watermark);
        cfg.keybind_list = get_b("keybind_list", cfg.keybind_list);
        misc::cfg.spectator_list = get_b("spectator_list", misc::cfg.spectator_list);
        misc::cfg.bomb_info = get_b("bomb_info", misc::cfg.bomb_info);

        auto load_esp = [&](const std::string& prefix, visuals::EspTargetSettings& esp) {
            esp.box = get_b(prefix + "_box", esp.box);
            get_col(prefix + "_box_color", esp.box_color);

            esp.health = get_b(prefix + "_health", esp.health);
            esp.health_gradient = get_b(prefix + "_health_gradient", esp.health_gradient);
            get_col(prefix + "_health_gradient_color", esp.health_gradient_color);
            esp.health_outline = get_b(prefix + "_health_outline", esp.health_outline);
            get_col(prefix + "_health_outline_color", esp.health_outline_color);
            esp.health_background = get_b(prefix + "_health_background", esp.health_background);
            get_col(prefix + "_health_bg_color", esp.health_bg_color);
        };

        auto load_chams = [&](const std::string& prefix, visuals::ChamsTargetSettings& chams) {
            chams.enabled = get_b(prefix + "_chams_enabled", chams.enabled);
            chams.material = get_i(prefix + "_chams_material", chams.material);
            get_col(prefix + "_chams_visible_color", chams.visible_color);
            chams.occluded = get_b(prefix + "_chams_occluded", chams.occluded);
            get_col(prefix + "_chams_occluded_color", chams.occluded_color);
        };

        load_esp("enemy", visuals::cfg.enemy);
        load_esp("team", visuals::cfg.team);
        load_esp("local", visuals::cfg.local);

        load_chams("enemy", visuals::cfg.chams_enemy);
        load_chams("team", visuals::cfg.chams_team);
        load_chams("local", visuals::cfg.chams_local);
        load_chams("arms", visuals::cfg.chams_arms);
        load_chams("weapon", visuals::cfg.chams_weapon);

        load_chams("ragdoll_enemy", visuals::cfg.chams_ragdoll_enemy);
        load_chams("ragdoll_team", visuals::cfg.chams_ragdoll_team);
        load_chams("ragdoll_local", visuals::cfg.chams_ragdoll_local);

        static const std::string weapon_prefixes_load[6] = { "pistol", "smg", "shotgun", "rifle", "sniper", "lmg" };
        for (int i = 0; i < 6; ++i) {
            auto& w = legit::cfg.weapons[i];
            std::string prefix = "legit_" + weapon_prefixes_load[i] + "_";
            w.enabled = get_b(prefix + "enabled", w.enabled);
            w.key = get_i(prefix + "key", w.key);
            w.key_mode = get_i(prefix + "key_mode", w.key_mode);
            w.target_part = get_i(prefix + "target_part", w.target_part);
            w.fov = get_f(prefix + "fov", w.fov);
            w.smooth = get_f(prefix + "smooth", w.smooth);
            w.draw_fov = get_b(prefix + "draw_fov", w.draw_fov);
            get_col(prefix + "fov_color", w.fov_color);

            w.trigger_enabled = get_b(prefix + "trigger_enabled", w.trigger_enabled);
            w.trigger_key = get_i(prefix + "trigger_key", w.trigger_key);
            w.trigger_key_mode = get_i(prefix + "trigger_key_mode", w.trigger_key_mode);
            w.trigger_filter_enabled = get_b(prefix + "trigger_filter_enabled", w.trigger_filter_enabled);
            w.trigger_target_part = get_i(prefix + "trigger_target_part", w.trigger_target_part);
            w.trigger_delay = get_i(prefix + "trigger_delay", w.trigger_delay);
            w.trigger_seeded_delay = get_b(prefix + "trigger_seeded_delay", w.trigger_seeded_delay);
        }

        skins::cfg.enabled = get_b("skins_enabled", skins::cfg.enabled);

        skins::cfg.equipped_knife_def = static_cast<int16_t>(get_i("skins_knife_def", skins::cfg.equipped_knife_def));
        skins::cfg.equipped_knife_skin.paint_kit_id = get_i("skins_knife_paint_kit", skins::cfg.equipped_knife_skin.paint_kit_id);
        skins::cfg.equipped_knife_skin.wear = get_f("skins_knife_wear", skins::cfg.equipped_knife_skin.wear);
        skins::cfg.equipped_knife_skin.seed = get_i("skins_knife_seed", skins::cfg.equipped_knife_skin.seed);
        skins::cfg.equipped_knife_skin.stattrak = get_b("skins_knife_stattrak", skins::cfg.equipped_knife_skin.stattrak);
        skins::cfg.equipped_knife_skin.stattrak_count = get_i("skins_knife_stattrak_count", skins::cfg.equipped_knife_skin.stattrak_count);

        skins::cfg.equipped_glove_def = static_cast<int16_t>(get_i("skins_glove_def", skins::cfg.equipped_glove_def));
        skins::cfg.equipped_glove_skin.paint_kit_id = get_i("skins_glove_paint_kit", skins::cfg.equipped_glove_skin.paint_kit_id);
        skins::cfg.equipped_glove_skin.wear = get_f("skins_glove_wear", skins::cfg.equipped_glove_skin.wear);
        skins::cfg.equipped_glove_skin.seed = get_i("skins_glove_seed", skins::cfg.equipped_glove_skin.seed);
        skins::cfg.equipped_glove_skin.stattrak = get_b("skins_glove_stattrak", skins::cfg.equipped_glove_skin.stattrak);
        skins::cfg.equipped_glove_skin.stattrak_count = get_i("skins_glove_stattrak_count", skins::cfg.equipped_glove_skin.stattrak_count);

        skins::cfg.equipped_agent_ct = static_cast<int16_t>(get_i("skins_agent_ct", skins::cfg.equipped_agent_ct));
        skins::cfg.equipped_agent_t = static_cast<int16_t>(get_i("skins_agent_t", skins::cfg.equipped_agent_t));

        skins::cfg.weapon_skins.clear();
        int w_count = get_i("skins_weapon_count", 0);
        for (int i = 0; i < w_count; ++i) {
            std::string prefix = "skins_weapon_" + std::to_string(i) + "_";
            int16_t def_idx = static_cast<int16_t>(get_i(prefix + "def", 0));
            if (def_idx > 0) {
                skins::AppliedSkin s;
                s.paint_kit_id = get_i(prefix + "paint_kit", 0);
                s.wear = get_f(prefix + "wear", 0.001f);
                s.seed = get_i(prefix + "seed", 1);
                s.stattrak = get_b(prefix + "stattrak", false);
                s.stattrak_count = get_i(prefix + "stattrak_count", 1337);
                skins::cfg.weapon_skins[def_idx] = s;
            }
        }
    }

    void delete_config(const std::string& name) {
        if (name.empty()) return;
        std::string path = "configs\\" + name + ".cfg";
        DeleteFileA(path.c_str());

        auto it = std::find(configs.begin(), configs.end(), name);
        if (it != configs.end()) {
            configs.erase(it);
        }
    }

    void render() {
        float full_w = ImGui::GetContentRegionAvail().x;
        float col_w = (full_w - 6.0f) * 0.5f;

        ImGui::BeginGroup();
        if (widgets::begin_section("Cheat Settings", col_w, 0.0f)) {
            if (widgets::toggle_with_color("Menu Accent", &cfg.menu_accent, cfg.accent_color, "Accent Color")) {
                gui::theme::accent_color = ImVec4(cfg.accent_color[0], cfg.accent_color[1], cfg.accent_color[2], cfg.accent_color[3]);
                gui::theme::apply();
            }

            widgets::toggle_with_keybind("Menu Bind", &cfg.menu_bind, &cfg.menu_key, &cfg.menu_key_mode);
            widgets::dropdown("Font", &cfg.selected_font, gui::theme::font_names, (int)gui::theme::FontID::Count);
            widgets::toggle("Watermark", &cfg.watermark);
            widgets::toggle("Keybind List", &cfg.keybind_list);
            widgets::toggle("Custom Menu Name", &cfg.custom_menu_name);
            if (cfg.custom_menu_name) {
                widgets::textbox("##menu_name", cfg.menu_name, sizeof(cfg.menu_name));
            }

            widgets::end_section();
        }
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 6.0f);

        ImGui::BeginGroup();
        if (widgets::begin_section("Configuration", col_w, 0.0f)) {
            widgets::textbox("Name", cfg.config_name, sizeof(cfg.config_name));

            std::vector<const char*> config_ptrs;
            for (const auto& c : configs) {
                config_ptrs.push_back(c.c_str());
            }

            if (cfg.selected_config >= (int)configs.size()) {
                cfg.selected_config = 0;
            }

            if (!config_ptrs.empty()) {
                widgets::dropdown("Configs", &cfg.selected_config, config_ptrs.data(), (int)config_ptrs.size());
            }

            ImGui::Spacing();
            if (widgets::button("Load Config")) {
                if (cfg.selected_config >= 0 && cfg.selected_config < (int)configs.size()) {
                    std::string target = configs[cfg.selected_config];
                    load_config(target);
                    gui::notifications::push("Configs", ("Loaded config: " + target).c_str(), gui::notifications::Type::Info);
                }
            }

            if (widgets::button("Save Config")) {
                std::string target = (cfg.config_name[0] != '\0') ? cfg.config_name : (cfg.selected_config < (int)configs.size() ? configs[cfg.selected_config] : "Default");
                save_config(target);
                
                for (size_t i = 0; i < configs.size(); i++) {
                    if (configs[i] == target) {
                        cfg.selected_config = (int)i;
                        break;
                    }
                }
                gui::notifications::push("Configs", ("Saved config: " + target).c_str(), gui::notifications::Type::Success);
            }

            if (widgets::button("Delete Config")) {
                if (configs.size() > 1 && cfg.selected_config >= 0 && cfg.selected_config < (int)configs.size()) {
                    std::string deleted = configs[cfg.selected_config];
                    delete_config(deleted);
                    cfg.selected_config = std::max(0, cfg.selected_config - 1);
                    cfg.selected_config = (std::max)(0, cfg.selected_config - 1);
                    gui::notifications::push("Configs", ("Deleted config: " + deleted).c_str(), gui::notifications::Type::Warning);
                }
            }

            widgets::end_section();
        }
        ImGui::EndGroup();
    }

}

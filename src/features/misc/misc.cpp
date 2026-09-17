
#include "misc.hpp"
#include "../../widgets/custom_widgets.hpp"
#include "../../gui/gui/gui.hpp"
#include "../../gui/theme/theme.hpp"
#include "../../core/interfaces/interfaces.hpp"
#include "../../core/memory/memory.hpp"
#include "../../valve/schema/schema.hpp"
#include "../../valve/sdk.hpp"
#include "../../valve/entity/entity.hpp"
#include "../../features/skins/skins.hpp"

namespace features::misc {

    static int current_subtab = 0;
    static const char* const subtab_names[] = { "Misc", "Skins" };

    std::vector<SpectatorInfo> get_spectators() {
        std::vector<SpectatorInfo> specs;

        uintptr_t local_ctrl = valve::entity::get_local_player_controller();
        if (!local_ctrl) return specs;

        static uint32_t off_hPlayerPawn = 0;
        static uint32_t off_hObserverPawn = 0;
        static uint32_t off_pObserverServices = 0;
        static uint32_t off_iObserverMode = 0;
        static uint32_t off_hObserverTarget = 0;
        static uint32_t off_steamID = 0;

        if (!off_hPlayerPawn) {
            off_hPlayerPawn = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_hPlayerPawn"));
            if (!off_hPlayerPawn) off_hPlayerPawn = 0x914;
        }
        if (!off_hObserverPawn) {
            off_hObserverPawn = schema::lookup("CCSPlayerController", fnv1a::runtime_hash("m_hObserverPawn"));
            if (!off_hObserverPawn) off_hObserverPawn = 0x918;
        }
        if (!off_pObserverServices) {
            off_pObserverServices = schema::lookup("C_BasePlayerPawn", fnv1a::runtime_hash("m_pObserverServices"));
            if (!off_pObserverServices) off_pObserverServices = 0x1220;
        }
        if (!off_iObserverMode) {
            off_iObserverMode = schema::lookup("CPlayer_ObserverServices", fnv1a::runtime_hash("m_iObserverMode"));
            if (!off_iObserverMode) off_iObserverMode = 0x48;
        }
        if (!off_hObserverTarget) {
            off_hObserverTarget = schema::lookup("CPlayer_ObserverServices", fnv1a::runtime_hash("m_hObserverTarget"));
            if (!off_hObserverTarget) off_hObserverTarget = 0x4C;
        }
        if (!off_steamID) {
            off_steamID = schema::lookup("CBasePlayerController", fnv1a::runtime_hash("m_steamID"));
            if (!off_steamID) off_steamID = 0x780;
        }

        auto get_obs_pawn = [&](uintptr_t ctrl) -> uintptr_t {
            if (!ctrl) return 0;
            uint32_t obs_handle = memory::read<uint32_t>(ctrl + off_hObserverPawn);
            uintptr_t obs_pawn = valve::entity::get_entity_by_handle(obs_handle);
            if (obs_pawn) return obs_pawn;

            uint32_t player_handle = memory::read<uint32_t>(ctrl + off_hPlayerPawn);
            return valve::entity::get_entity_by_handle(player_handle);
        };

        auto get_obs_target = [&](uintptr_t pawn) -> uintptr_t {
            if (!pawn) return 0;
            uintptr_t services = memory::read<uintptr_t>(pawn + off_pObserverServices);
            if (!services) return 0;

            uint8_t mode = memory::read<uint8_t>(services + off_iObserverMode);
            if (!mode) return 0;

            uint32_t target_handle = memory::read<uint32_t>(services + off_hObserverTarget);
            if (!target_handle || target_handle == 0xFFFFFFFF || target_handle == 0xFFFFFFFE) return 0;

            return valve::entity::get_entity_by_handle(target_handle);
        };

        uintptr_t local_obs_pawn = get_obs_pawn(local_ctrl);
        uintptr_t spectator_target = get_obs_target(local_obs_pawn);

        bool local_is_spectator = (spectator_target != 0);
        uintptr_t watched_pawn = local_is_spectator ? spectator_target : local_obs_pawn;
        if (!watched_pawn) return specs;

        auto push_unique = [&](const std::string& name, uint64_t steam_id) {
            if (name.empty()) return;
            for (const auto& existing : specs) {
                if (existing.name == name) return;
            }
            specs.push_back({ name, steam_id });
        };

        for (int i = 1; i <= 64; ++i) {
            uintptr_t entity = valve::entity::get_entity_by_index(i);
            if (!entity || entity == local_ctrl) continue;

            uintptr_t p_obs_pawn = get_obs_pawn(entity);
            if (!p_obs_pawn) continue;

            uintptr_t target = get_obs_target(p_obs_pawn);
            if (target != watched_pawn) continue;

            std::string name = valve::entity::get_player_name(entity);
            if (name.empty()) name = "Spectator";

            uint64_t sid = off_steamID ? memory::read<uint64_t>(entity + off_steamID) : 0;
            push_unique(name, sid);
        }

        if (local_is_spectator) {
            std::string name = valve::entity::get_player_name(local_ctrl);
            if (name.empty()) name = "you";
            uint64_t sid = off_steamID ? memory::read<uint64_t>(local_ctrl + off_steamID) : 0;
            push_unique(name, sid);
        }

        return specs;
    }

    void render() {
        widgets::subtab_bar(&current_subtab, subtab_names, IM_ARRAYSIZE(subtab_names));

        float full_w = ImGui::GetContentRegionAvail().x;
        float col_w = (full_w - 6.0f) * 0.5f;

        if (current_subtab == 0) { 
            ImGui::BeginGroup();
            if (widgets::begin_section("Display", col_w, 0.0f)) {
                widgets::toggle("Spectator List", &cfg.spectator_list);
                widgets::toggle("Bomb Info", &cfg.bomb_info);

                widgets::end_section();
            }
            ImGui::EndGroup();
        } else if (current_subtab == 1) { 
            features::skins::render_ui();
        }
    }

}


#include "hooks.hpp"
#include "../interfaces/interfaces.hpp"
#include "../memory/memory.hpp"
#include "../../gui/gui/gui.hpp"
#include "../../gui/theme/theme.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/visuals/materials/materials.hpp"
#include "../../features/skins/skins.hpp"
#include "../../features/skins/skin_image.hpp"
#include "../../features/skins/vpk_vtex.hpp"
#include "../../valve/entity/entity.hpp"
#include "../../valve/icons/icons.hpp"
#include "../../valve/steam/steam_avatar.hpp"
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx11.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(STDMETHODCALLTYPE* PresentFn)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

namespace hooks {
    bool is_menu_open = true;
}

static WNDPROC original_wndproc = nullptr;
static HWND hooked_window = nullptr;

static ID3D11Device* d3d_device = nullptr;
static ID3D11DeviceContext* d3d_context = nullptr;
static ID3D11RenderTargetView* render_target = nullptr;
static bool imgui_initialized = false;


static PresentFn original_present = nullptr;
static ResizeBuffersFn original_resize = nullptr;

static void setup_rtv(IDXGISwapChain* sc) {
    if (render_target) { render_target->Release(); render_target = nullptr; }
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back))) {
        d3d_device->CreateRenderTargetView(back, nullptr, &render_target);
        back->Release();
    }
}

static void ensure_wndproc(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;

    static unsigned tick = 0;
    if (hooked_window == hwnd && original_wndproc && ((++tick) & 31u) != 0) return;

    WNDPROC cur = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    if (cur == hooks::hk_wndproc) return;

    if (hooked_window && hooked_window != hwnd && original_wndproc && IsWindow(hooked_window))
        SetWindowLongPtrW(hooked_window, GWLP_WNDPROC, (LONG_PTR)original_wndproc);

    original_wndproc = cur;
    hooked_window = hwnd;
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)hooks::hk_wndproc);
}

HRESULT STDMETHODCALLTYPE hooks::hk_present(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!imgui_initialized) {
        if (SUCCEEDED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&d3d_device))) {
            d3d_device->GetImmediateContext(&d3d_context);
            setup_rtv(sc);

            ImGui::CreateContext();
            gui::init();

            DXGI_SWAP_CHAIN_DESC desc{};
            sc->GetDesc(&desc);
            ImGui_ImplWin32_Init(desc.OutputWindow);
            ImGui_ImplDX11_Init(d3d_device, d3d_context);

            valve::icons::initialize(d3d_device);
            valve::steam::set_d3d_device(d3d_device);
            features::skins::skin_img::init(d3d_device);
            features::skins::vpk_vtex::init(d3d_device);


            imgui_initialized = true;
        }
    }

    if (imgui_initialized) {
        DXGI_SWAP_CHAIN_DESC desc{};
        sc->GetDesc(&desc);
        ensure_wndproc(desc.OutputWindow);

        if (gui::menu_open && interfaces::input_system) {
            memory::call_vfunc<void>(interfaces::input_system, 76, false);
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        features::visuals::draw_esp();
        features::skins::update();
        gui::render();

        ImGui::Render();
        d3d_context->OMSetRenderTargets(1, &render_target, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    
    return original_present(sc, sync, flags);
}

HRESULT STDMETHODCALLTYPE hooks::hk_resize_buffers(IDXGISwapChain* sc, UINT bc, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
    if (imgui_initialized) {
        ImGui_ImplDX11_InvalidateDeviceObjects();
        if (render_target) { render_target->Release(); render_target = nullptr; }
    }

    HRESULT hr = original_resize(sc, bc, w, h, fmt, flags);

    if (SUCCEEDED(hr) && imgui_initialized) {
        setup_rtv(sc);
        ImGui_ImplDX11_CreateDeviceObjects();
    }

    return hr;
}

LRESULT CALLBACK hooks::hk_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_ACTIVATE && LOWORD(wparam) != WA_INACTIVE && gui::menu_open) {
        if (interfaces::input_system)
            memory::call_vfunc<void>(interfaces::input_system, 76, false);
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }

    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

    if (gui::menu_open) {
        switch (msg) {
        case WM_MOUSEMOVE:
        case WM_INPUT:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            return 0;
        }
    }

    return original_wndproc
        ? CallWindowProcW(original_wndproc, hwnd, msg, wparam, lparam)
        : DefWindowProcW(hwnd, msg, wparam, lparam);
}

typedef void(__fastcall* GeneratePrimitivesFn)(void*, uintptr_t, uintptr_t, uintptr_t);
static GeneratePrimitivesFn original_generate_primitives = nullptr;

static bool detour_hook(void* target, void* detour, void** original) {
    if (!target || !detour || !original) return false;

    constexpr size_t patch_size = 14;
    void* trampoline = VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return false;

    memcpy(trampoline, target, patch_size);

    uint8_t* tramp_jmp = reinterpret_cast<uint8_t*>(trampoline) + patch_size;
    tramp_jmp[0] = 0xFF;
    tramp_jmp[1] = 0x25;
    *reinterpret_cast<int32_t*>(tramp_jmp + 2) = 0;
    *reinterpret_cast<uintptr_t*>(tramp_jmp + 6) = reinterpret_cast<uintptr_t>(target) + patch_size;

    *original = trampoline;

    DWORD old_protect;
    VirtualProtect(target, patch_size, PAGE_EXECUTE_READWRITE, &old_protect);

    uint8_t* target_patch = reinterpret_cast<uint8_t*>(target);
    target_patch[0] = 0xFF;
    target_patch[1] = 0x25;
    *reinterpret_cast<int32_t*>(target_patch + 2) = 0;
    *reinterpret_cast<uintptr_t*>(target_patch + 6) = reinterpret_cast<uintptr_t>(detour);

    VirtualProtect(target, patch_size, old_protect, &old_protect);
    return true;
}

static int get_primitive_count(uintptr_t buf) {
    if (!buf || IsBadReadPtr(reinterpret_cast<void*>(buf), 0x20)) return 0;
    int fixed_count = memory::read<int>(buf + 0xC);
    int overflow_count = memory::read<int>(buf + 0x10);
    if (fixed_count < 0) fixed_count = 0;
    if (overflow_count < 0) overflow_count = 0;
    return fixed_count + overflow_count;
}

static uintptr_t get_primitive_address(uintptr_t buf, int index) {
    if (!buf || IsBadReadPtr(reinterpret_cast<void*>(buf), 0x20)) return 0;
    int fixed_count = memory::read<int>(buf + 0xC);
    if (index < fixed_count) {
        uintptr_t fixed_data = memory::read<uintptr_t>(buf + 0x0);
        return fixed_data ? (fixed_data + static_cast<size_t>(index) * 0x70) : 0;
    }
    int overflow_count = memory::read<int>(buf + 0x10);
    if (index < fixed_count + overflow_count) {
        uintptr_t overflow_data = memory::read<uintptr_t>(buf + 0x18);
        return overflow_data ? (overflow_data + static_cast<size_t>(index - fixed_count) * 0x70) : 0;
    }
    return 0;
}

static void __fastcall hk_generate_primitives(void* thisptr, uintptr_t scene_object, uintptr_t scene_view, uintptr_t primitive_buffer) {
    if (scene_object && !IsBadReadPtr(reinterpret_cast<void*>(scene_object), 0xD0)) {
        uint32_t owner_handle = memory::read<uint32_t>(scene_object + 0xC0);
        uintptr_t owner_entity = valve::entity::get_entity_by_handle(owner_handle);

        if (owner_entity) {
            const char* schema_name = valve::entity::get_schema_name(owner_entity);
            if (schema_name) {
                bool is_player = (strcmp(schema_name, "C_CSPlayerPawn") == 0);
                bool is_ragdoll = (strcmp(schema_name, "C_CSRagdoll") == 0);
                bool is_arms = (strcmp(schema_name, "C_CS2HudModelArms") == 0);
                bool is_weapon = (strcmp(schema_name, "C_CS2HudModelWeapon") == 0) || (strstr(schema_name, "C_Weapon") != nullptr) || (strstr(schema_name, "C_Knife") != nullptr) || (strcmp(schema_name, "C_DEagle") == 0) || (strcmp(schema_name, "C_AK47") == 0);

                if (is_player || is_ragdoll || is_arms || is_weapon) {
                    uintptr_t local_pawn = valve::entity::get_local_player_pawn();
                    uint8_t local_team = valve::entity::get_team(local_pawn);
                    uint8_t team = valve::entity::get_team(owner_entity);

                    bool is_ffa = features::visuals::is_ffa_mode();
                    bool is_local = (owner_entity == local_pawn);
                    bool is_enemy = !is_local && (is_ffa || (local_team != 0 ? (team != local_team) : true));

                    const auto& cfg = features::visuals::cfg;
                    const features::visuals::ChamsTargetSettings* chams_cfg = nullptr;

                    if (is_player || is_ragdoll) {
                        int health = valve::entity::get_health(owner_entity);
                        uint8_t life_state = valve::entity::get_life_state(owner_entity);
                        bool is_dead = is_ragdoll || (health <= 0) || (life_state != 0);

                        if (is_dead) {
                            chams_cfg = is_local ? &cfg.chams_ragdoll_local : (is_enemy ? &cfg.chams_ragdoll_enemy : &cfg.chams_ragdoll_team);
                        } else {
                            chams_cfg = is_local ? &cfg.chams_local : (is_enemy ? &cfg.chams_enemy : &cfg.chams_team);
                        }
                    } else if (is_arms) {
                        chams_cfg = &cfg.chams_arms;
                    } else if (is_weapon) {
                        chams_cfg = &cfg.chams_weapon;
                    }

                    if (chams_cfg && (chams_cfg->enabled || chams_cfg->occluded)) {
                        materials::initialize();

                        uint8_t flags = memory::read<uint8_t>(scene_object + 0x78);
                        if (flags) {
                            memory::write<uint8_t>(scene_object + 0x78, static_cast<uint8_t>(flags & ~(1u << 3)));
                        }

                        auto apply_pass = [&](bool is_occluded_pass) {
                            const float* col = is_occluded_pass ? chams_cfg->occluded_color : chams_cfg->visible_color;
                            uintptr_t mat_handle = materials::find(chams_cfg->material, is_occluded_pass);

                            int start_idx = get_primitive_count(primitive_buffer);
                            original_generate_primitives(thisptr, scene_object, scene_view, primitive_buffer);
                            int end_idx = get_primitive_count(primitive_buffer);

                            if (end_idx > start_idx) {
                                uint8_t r = static_cast<uint8_t>(col[0] * 255.0f);
                                uint8_t g = static_cast<uint8_t>(col[1] * 255.0f);
                                uint8_t b = static_cast<uint8_t>(col[2] * 255.0f);
                                uint8_t a = static_cast<uint8_t>(col[3] * 255.0f);
                                uint32_t col_bytes = (a << 24) | (b << 16) | (g << 8) | r;

                                for (int i = start_idx; i < end_idx; i++) {
                                    uintptr_t prim = get_primitive_address(primitive_buffer, i);
                                    if (prim && !IsBadReadPtr(reinterpret_cast<void*>(prim), 0x70)) {
                                        if (mat_handle) {
                                            memory::write<uintptr_t>(prim + 0x20, mat_handle);
                                            memory::write<uintptr_t>(prim + 0x28, mat_handle);
                                        }
                                        memory::write<uint32_t>(prim + 0x50, col_bytes);
                                    }
                                }
                            }
                        };

                        if (chams_cfg->occluded) {
                            apply_pass(true);
                        }
                        if (chams_cfg->enabled) {
                            apply_pass(false);
                        }
                        return;
                    }
                }
            }
        }
    }

    original_generate_primitives(thisptr, scene_object, scene_view, primitive_buffer);
}

namespace hooks {

    bool initialize() {
        uintptr_t* vtbl = interfaces::swap_chain_vtable;
        if (!vtbl) {
            return false;
        }

        materials::initialize();

        original_present = (PresentFn)vtbl[8];
        original_resize  = (ResizeBuffersFn)vtbl[13];

        DWORD old;

        VirtualProtect(&vtbl[8], sizeof(uintptr_t), PAGE_READWRITE, &old);
        vtbl[8] = (uintptr_t)hk_present;
        VirtualProtect(&vtbl[8], sizeof(uintptr_t), old, &old);

        VirtualProtect(&vtbl[13], sizeof(uintptr_t), PAGE_READWRITE, &old);
        vtbl[13] = (uintptr_t)hk_resize_buffers;
        VirtualProtect(&vtbl[13], sizeof(uintptr_t), old, &old);

        uintptr_t gen_prim_addr = memory::pattern_scan("scenesystem.dll", "48 8B C4 48 89 58 08 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FF FF FF 48 81 EC 90 01 00 00");
        if (gen_prim_addr) {
            detour_hook(reinterpret_cast<void*>(gen_prim_addr), reinterpret_cast<void*>(hk_generate_primitives), reinterpret_cast<void**>(&original_generate_primitives));
        }

        return true;
    }

    void shutdown() {
        
        uintptr_t* vtbl = interfaces::swap_chain_vtable;
        if (vtbl && original_present && original_resize) {
            DWORD old;
            VirtualProtect(&vtbl[8], sizeof(uintptr_t), PAGE_READWRITE, &old);
            vtbl[8] = (uintptr_t)original_present;
            VirtualProtect(&vtbl[8], sizeof(uintptr_t), old, &old);

            VirtualProtect(&vtbl[13], sizeof(uintptr_t), PAGE_READWRITE, &old);
            vtbl[13] = (uintptr_t)original_resize;
            VirtualProtect(&vtbl[13], sizeof(uintptr_t), old, &old);
        }

        if (hooked_window && original_wndproc && IsWindow(hooked_window))
            SetWindowLongPtrW(hooked_window, GWLP_WNDPROC, (LONG_PTR)original_wndproc);

        if (imgui_initialized) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        if (render_target) render_target->Release();
        if (d3d_context) d3d_context->Release();
        if (d3d_device) d3d_device->Release();
    }

}

#include "interfaces.hpp"
#include "../memory/memory.hpp"
#include <d3d11.h>
#include <dxgi.h>

typedef void* (*CreateInterfaceFn)(const char* name, int* return_code);

static void* get_interface(const char* module_name, const char* iface_name) {
    HMODULE h = GetModuleHandleA(module_name);
    if (!h) return nullptr;
    auto fn = (CreateInterfaceFn)GetProcAddress(h, "CreateInterface");
    if (!fn) return nullptr;
    return fn(iface_name, nullptr);
}

static bool get_dx11_vtable(uintptr_t& present_out, uintptr_t& resize_out, uintptr_t*& vtable_out) {
    HMODULE inst = interfaces::dll_module;

    char cls[64];
    snprintf(cls, sizeof(cls), "aura_%lu_%lu", GetCurrentProcessId(), GetTickCount());

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = inst ? inst : GetModuleHandleA(nullptr);
    wc.lpszClassName = cls;

    UnregisterClassA(cls, wc.hInstance);
    if (!RegisterClassExA(&wc)) {
        return false;
    }

    HWND wnd = CreateWindowExA(0, cls, "", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    if (!wnd) {
        UnregisterClassA(cls, wc.hInstance);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Width = 64;
    desc.BufferDesc.Height = 64;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = wnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &desc, &sc, &dev, nullptr, &ctx);
    if (SUCCEEDED(hr) && sc) {
        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(sc);
        present_out = vtbl[8];
        resize_out = vtbl[13];
        vtable_out = vtbl;
    }

    if (ctx) ctx->Release();
    if (dev) dev->Release();
    if (sc) sc->Release();

    DestroyWindow(wnd);
    UnregisterClassA(cls, wc.hInstance);
    return present_out != 0 && resize_out != 0;
}

namespace interfaces {

    HMODULE dll_module = nullptr;

    void* schema_system = nullptr;
    void* engine_client = nullptr;
    void* entity_system = nullptr;
    void* input_system = nullptr;

    uintptr_t entity_list = 0;
    uintptr_t local_player_controller = 0;
    uintptr_t view_matrix_ptr = 0;
    uintptr_t global_vars = 0;
    uintptr_t planted_c4 = 0;

    uintptr_t present_address = 0;
    uintptr_t resize_buffers_address = 0;
    uintptr_t* swap_chain_vtable = nullptr;

    bool initialize() {
        schema_system = get_interface("schemasystem.dll", "SchemaSystem_001");
        engine_client = get_interface("engine2.dll", "Source2EngineToClient001");
        input_system = get_interface("inputsystem.dll", "InputSystemVersion001");

        
        uintptr_t ent_list_addr = memory::pattern_scan("client.dll", "48 8B 0D ? ? ? ? 8B FB C1 EB 0E");
        if (ent_list_addr) {
            entity_list = memory::resolve_rip(ent_list_addr, 3, 7);
        }

        uintptr_t local_ctrl_addr = memory::pattern_scan("client.dll", "48 39 1D ? ? ? ? 75 04 B0 01");
        if (local_ctrl_addr) {
            local_player_controller = memory::resolve_rip(local_ctrl_addr, 3, 7);
        }

        uintptr_t view_matrix_addr = memory::pattern_scan("client.dll", "48 8D 0D ? ? ? ? 48 C1 E0 06");
        if (view_matrix_addr) {
            view_matrix_ptr = memory::resolve_rip(view_matrix_addr, 3, 7);
        }

        uintptr_t gvars_addr = memory::pattern_scan("client.dll", "48 8B 05 ? ? ? ? 44 8B 40 44");
        if (gvars_addr) {
            global_vars = memory::resolve_rip(gvars_addr, 3, 7);
        }

        uintptr_t c4_addr = memory::pattern_scan("client.dll", "48 8B 1D ? ? ? ? 48 8B D3 4C 8B 81");
        if (c4_addr) {
            planted_c4 = memory::resolve_rip(c4_addr, 3, 7);
        }

        if (!get_dx11_vtable(present_address, resize_buffers_address, swap_chain_vtable))
            return false;

        return true;
    }

}

#include "window.hpp"
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_win32.h"
#include "../../imgui/imgui_impl_dx11.h"
#include "../../gui/gui/gui.hpp"
#include "../../gui/theme/theme.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace core::window {

    static LRESULT WINAPI wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg) {
            case WM_SIZE:
                if (d3d_device != nullptr && wParam != SIZE_MINIMIZED) {
                    cleanup_render_target();
                    swap_chain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                    create_render_target();
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)
                    return 0;
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    bool init_dx11() {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT create_device_flags = 0;
        D3D_FEATURE_LEVEL feature_level;
        const D3D_FEATURE_LEVEL feature_levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
        
        HRESULT res = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            create_device_flags,
            feature_levels,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &swap_chain,
            &d3d_device,
            &feature_level,
            &d3d_context
        );

        if (res == DXGI_ERROR_UNSUPPORTED) {
            res = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                create_device_flags,
                feature_levels,
                2,
                D3D11_SDK_VERSION,
                &sd,
                &swap_chain,
                &d3d_device,
                &feature_level,
                &d3d_context
            );
        }

        if (FAILED(res))
            return false;

        create_render_target();
        return true;
    }

    void cleanup_dx11() {
        cleanup_render_target();
        if (swap_chain) {
            swap_chain->Release();
            swap_chain = nullptr;
        }
        if (d3d_context) {
            d3d_context->Release();
            d3d_context = nullptr;
        }
        if (d3d_device) {
            d3d_device->Release();
            d3d_device = nullptr;
        }
    }

    void create_render_target() {
        ID3D11Texture2D* back_buffer = nullptr;
        swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
        if (back_buffer) {
            d3d_device->CreateRenderTargetView(back_buffer, nullptr, &render_target);
            back_buffer->Release();
        }
    }

    void cleanup_render_target() {
        if (render_target) {
            render_target->Release();
            render_target = nullptr;
        }
    }

    bool create(int width, int height, const char* title) {
        WNDCLASSEXA wc = {
            sizeof(wc),
            CS_CLASSDC,
            wnd_proc,
            0L,
            0L,
            GetModuleHandle(nullptr),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            "aurahook_wnd_class",
            nullptr
        };
        RegisterClassExA(&wc);

        int screen_w = GetSystemMetrics(SM_CXSCREEN);
        int screen_h = GetSystemMetrics(SM_CYSCREEN);
        int pos_x = (screen_w - width) / 2;
        int pos_y = (screen_h - height) / 2;

        hwnd = CreateWindowA(
            wc.lpszClassName,
            title,
            WS_POPUP | WS_VISIBLE,
            pos_x,
            pos_y,
            width,
            height,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );

        if (!hwnd) {
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            return false;
        }

        if (!init_dx11()) {
            cleanup_dx11();
            UnregisterClassA(wc.lpszClassName, wc.hInstance);
            return false;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        struct FontConfig {
            const char* filename;
            float main_size;
            float tab_size;
        };

        static const FontConfig font_configs[(int)gui::theme::FontID::Count] = {
            { "ProggyClean.ttf",         13.0f, 13.0f },
            { "Plex2.ttf",               15.0f, 16.0f },
            { "fs-tahoma-8px.ttf",       11.0f, 11.5f },
            { "smallest_pixel-7.ttf",    10.0f, 10.0f },
            { "Tahoma.ttf",              12.5f, 13.5f },
            { "minecraftia.ttf",         11.5f, 12.5f }
        };

        for (int i = 0; i < (int)gui::theme::FontID::Count; i++) {
            char font_path[MAX_PATH];
            snprintf(font_path, sizeof(font_path), "fonts\\%s", font_configs[i].filename);

            if (GetFileAttributesA(font_path) != INVALID_FILE_ATTRIBUTES) {
                gui::theme::fonts[i] = io.Fonts->AddFontFromFileTTF(font_path, font_configs[i].main_size);
                gui::theme::fonts_tabs[i] = io.Fonts->AddFontFromFileTTF(font_path, font_configs[i].tab_size);
            } else {
                gui::theme::fonts[i] = io.Fonts->AddFontDefault();
                gui::theme::fonts_tabs[i] = io.Fonts->AddFontDefault();
            }
        }

        if (gui::theme::fonts[0]) {
            io.FontDefault = gui::theme::fonts[0];
            gui::theme::font_main = gui::theme::fonts[0];
        } else {
            gui::theme::font_main = io.Fonts->AddFontDefault();
        }

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(d3d_device, d3d_context);

        gui::init();

        return true;
    }

    void destroy() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        cleanup_dx11();
        DestroyWindow(hwnd);
        UnregisterClassA("aurahook_wnd_class", GetModuleHandle(nullptr));
    }

    void run_message_loop() {
        bool done = false;
        const float clear_color[4] = { 12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.00f };

        while (!done) {
            MSG msg;
            while (PeekMessageA(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            gui::render();

            ImGui::Render();
            d3d_context->OMSetRenderTargets(1, &render_target, nullptr);
            d3d_context->ClearRenderTargetView(render_target, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            swap_chain->Present(1, 0); 
        }
    }

}

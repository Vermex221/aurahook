#pragma once

#include <windows.h>
#include <d3d11.h>


namespace core::window {

    inline HWND hwnd = nullptr;
    inline ID3D11Device* d3d_device = nullptr;
    inline ID3D11DeviceContext* d3d_context = nullptr;
    inline IDXGISwapChain* swap_chain = nullptr;
    inline ID3D11RenderTargetView* render_target = nullptr;

    bool create(int width = 1280, int height = 800, const char* title = "aurahook");
    void destroy();

    bool init_dx11();
    void cleanup_dx11();
    void create_render_target();
    void cleanup_render_target();

    void run_message_loop();

}

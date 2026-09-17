#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace hooks {

    extern bool is_menu_open;

    HRESULT STDMETHODCALLTYPE hk_present(IDXGISwapChain* sc, UINT sync, UINT flags);
    HRESULT STDMETHODCALLTYPE hk_resize_buffers(IDXGISwapChain* sc, UINT bc, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags);
    LRESULT CALLBACK         hk_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    bool initialize();
    void shutdown();

}

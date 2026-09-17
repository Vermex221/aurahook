
#include "steam_avatar.hpp"
#include <windows.h>
#include <unordered_map>
#include <vector>

namespace valve::steam {

    static ID3D11Device* g_device = nullptr;
    static std::unordered_map<uint64_t, ID3D11ShaderResourceView*> avatar_cache;

    void set_d3d_device(ID3D11Device* device) {
        g_device = device;
    }

    typedef void* (*SteamAPI_SteamFriends_Fn)();
    typedef void* (*SteamAPI_SteamUtils_Fn)();

    typedef int (*SteamAPI_ISteamFriends_GetAvatar_Fn)(void*, uint64_t);
    typedef bool (*SteamAPI_ISteamUtils_GetImageSize_Fn)(void*, int, uint32_t*, uint32_t*);
    typedef bool (*SteamAPI_ISteamUtils_GetImageRGBA_Fn)(void*, int, uint8_t*, int);

    ID3D11ShaderResourceView* get_avatar_texture(uint64_t steam_id) {
        if (!g_device || !steam_id) return nullptr;

        auto it = avatar_cache.find(steam_id);
        if (it != avatar_cache.end()) {
            return it->second;
        }

        HMODULE h_steam = GetModuleHandleA("steam_api64.dll");
        if (!h_steam) return nullptr;

        static SteamAPI_SteamFriends_Fn fn_friends = nullptr;
        static SteamAPI_SteamUtils_Fn fn_utils = nullptr;
        static SteamAPI_ISteamFriends_GetAvatar_Fn fn_c_get_medium = nullptr;
        static SteamAPI_ISteamFriends_GetAvatar_Fn fn_c_get_small = nullptr;
        static SteamAPI_ISteamFriends_GetAvatar_Fn fn_c_get_large = nullptr;
        static SteamAPI_ISteamUtils_GetImageSize_Fn fn_c_get_size = nullptr;
        static SteamAPI_ISteamUtils_GetImageRGBA_Fn fn_c_get_rgba = nullptr;

        if (!fn_friends) {
            fn_friends = (SteamAPI_SteamFriends_Fn)GetProcAddress(h_steam, "SteamAPI_SteamFriends_v018");
            if (!fn_friends) fn_friends = (SteamAPI_SteamFriends_Fn)GetProcAddress(h_steam, "SteamAPI_SteamFriends_v017");
            if (!fn_friends) fn_friends = (SteamAPI_SteamFriends_Fn)GetProcAddress(h_steam, "SteamAPI_SteamFriends");
        }
        if (!fn_utils) {
            fn_utils = (SteamAPI_SteamUtils_Fn)GetProcAddress(h_steam, "SteamAPI_SteamUtils_v010");
            if (!fn_utils) fn_utils = (SteamAPI_SteamUtils_Fn)GetProcAddress(h_steam, "SteamAPI_SteamUtils_v009");
            if (!fn_utils) fn_utils = (SteamAPI_SteamUtils_Fn)GetProcAddress(h_steam, "SteamAPI_SteamUtils");
        }
        if (!fn_c_get_medium) fn_c_get_medium = (SteamAPI_ISteamFriends_GetAvatar_Fn)GetProcAddress(h_steam, "SteamAPI_ISteamFriends_GetMediumFriendAvatar");
        if (!fn_c_get_small) fn_c_get_small = (SteamAPI_ISteamFriends_GetAvatar_Fn)GetProcAddress(h_steam, "SteamAPI_ISteamFriends_GetSmallFriendAvatar");
        if (!fn_c_get_large) fn_c_get_large = (SteamAPI_ISteamFriends_GetAvatar_Fn)GetProcAddress(h_steam, "SteamAPI_ISteamFriends_GetLargeFriendAvatar");
        if (!fn_c_get_size) fn_c_get_size = (SteamAPI_ISteamUtils_GetImageSize_Fn)GetProcAddress(h_steam, "SteamAPI_ISteamUtils_GetImageSize");
        if (!fn_c_get_rgba) fn_c_get_rgba = (SteamAPI_ISteamUtils_GetImageRGBA_Fn)GetProcAddress(h_steam, "SteamAPI_ISteamUtils_GetImageRGBA");

        if (!fn_friends || !fn_utils || !fn_c_get_size || !fn_c_get_rgba) return nullptr;

        void* friends = fn_friends();
        void* utils = fn_utils();
        if (!friends || !utils) return nullptr;

        int img_handle = 0;
        if (fn_c_get_medium) img_handle = fn_c_get_medium(friends, steam_id);
        if (img_handle <= 0 && fn_c_get_small) img_handle = fn_c_get_small(friends, steam_id);
        if (img_handle <= 0 && fn_c_get_large) img_handle = fn_c_get_large(friends, steam_id);

        if (img_handle <= 0) return nullptr;

        uint32_t w = 0, h = 0;
        if (!fn_c_get_size(utils, img_handle, &w, &h) || w == 0 || h == 0) return nullptr;

        std::vector<uint8_t> rgba(w * h * 4);
        if (!fn_c_get_rgba(utils, img_handle, rgba.data(), static_cast<int>(rgba.size()))) return nullptr;

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init_data{};
        init_data.pSysMem = rgba.data();
        init_data.SysMemPitch = w * 4;

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = g_device->CreateTexture2D(&desc, &init_data, &tex);
        if (FAILED(hr) || !tex) return nullptr;

        ID3D11ShaderResourceView* srv = nullptr;
        hr = g_device->CreateShaderResourceView(tex, nullptr, &srv);
        tex->Release();
        if (FAILED(hr) || !srv) return nullptr;

        avatar_cache[steam_id] = srv;
        return srv;
    }

    void cleanup_avatars() {
        for (auto& [id, srv] : avatar_cache) {
            if (srv) srv->Release();
        }
        avatar_cache.clear();
        g_device = nullptr;
    }

}

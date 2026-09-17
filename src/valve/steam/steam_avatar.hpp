
#pragma once

#include <cstdint>
#include <d3d11.h>

namespace valve::steam {
    void set_d3d_device(ID3D11Device* device);
    ID3D11ShaderResourceView* get_avatar_texture(uint64_t steam_id);
    void cleanup_avatars();
}


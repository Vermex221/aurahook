#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>
#include "../../valve/sdk.hpp"

namespace interfaces {

    extern HMODULE dll_module;

    extern void* schema_system;
    extern void* engine_client;
    extern void* entity_system;
    extern void* input_system;

    extern uintptr_t entity_list;
    extern uintptr_t local_player_controller;
    extern uintptr_t view_matrix_ptr;
    extern uintptr_t global_vars;
    extern uintptr_t planted_c4;

    extern uintptr_t present_address;
    extern uintptr_t resize_buffers_address;
    extern uintptr_t* swap_chain_vtable;

    bool initialize();

}

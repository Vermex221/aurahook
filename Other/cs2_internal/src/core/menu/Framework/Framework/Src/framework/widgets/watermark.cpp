// Created by Valorr19
// watermark.cpp

#include <core/common.hpp>
#include <core/settings.hpp>
#include "../headers/functions.h"
#include "../headers/widgets.h"
#include <modules/economy/economy.h>
#include <modules/economy/cloud_config.h>
#include <valve/classes/CSchemaSystem.h>
#include <valve/schemas/CBasePlayerController.h>
#include <algorithm>
#include <array>
#include <ctime>
#include <d3dcompiler.h>
#include <sstream>
#include <iomanip>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

namespace {

const char* blur_shader_hlsl = R"(
cbuffer BlurBuffer : register(b0)
{
    float2 resolution;
    float blurAmount;
    float padding;
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uv = input.uv;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target
{
    float2 texelSize = 1.0 / resolution * blurAmount;
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    float weights[9] = {
        0.0625, 0.125, 0.0625,
        0.125,  0.25,  0.125,
        0.0625, 0.125, 0.0625
    };

    int index = 0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            color += tex.Sample(samplerState, input.uv + offset) * weights[index++];
        }
    }

    return color;
}
)";

}

void c_watermark::initialize(ID3D11Device* device)
{
    if (initialized)
        return;
    
    try
    {
        create_blur_shader(device);
        initialized = true;
    }
    catch (...)
    {
        initialized = false;
    }
}

void c_watermark::start_timer()
{
    if (!timer_started)
    {
        injection_time = std::chrono::steady_clock::now();
        timer_started = true;
    }
}

void c_watermark::create_blur_shader(ID3D11Device* device)
{
    if (!device)
        return;
    
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    
    HRESULT hr = D3DCompile(
        blur_shader_hlsl,
        strlen(blur_shader_hlsl),
        nullptr,
        nullptr,
        nullptr,
        "VS",
        "vs_5_0",
        0,
        0,
        &vs_blob,
        &error_blob
    );
    
    if (SUCCEEDED(hr))
    {
        device->CreateVertexShader(
            vs_blob->GetBufferPointer(),
            vs_blob->GetBufferSize(),
            nullptr,
            &vertex_shader
        );
    }
    
    if (error_blob) error_blob->Release();
    error_blob = nullptr;
    
    hr = D3DCompile(
        blur_shader_hlsl,
        strlen(blur_shader_hlsl),
        nullptr,
        nullptr,
        nullptr,
        "PS",
        "ps_5_0",
        0,
        0,
        &ps_blob,
        &error_blob
    );
    
    if (SUCCEEDED(hr))
    {
        device->CreatePixelShader(
            ps_blob->GetBufferPointer(),
            ps_blob->GetBufferSize(),
            nullptr,
            &pixel_shader
        );
    }
    
    if (vs_blob) vs_blob->Release();
    if (ps_blob) ps_blob->Release();
    if (error_blob) error_blob->Release();
    
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.ByteWidth = sizeof(float) * 4; 
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    device->CreateBuffer(&cb_desc, nullptr, &constant_buffer);
    
    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    
    device->CreateSamplerState(&sampler_desc, &sampler_state);
    
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    device->CreateBlendState(&blend_desc, &blend_state);
}

void c_watermark::render()
{
    return; // temp disabled

    if (!gui || !draw || !font)
        return;

    const float spawn_alpha = var->gui.watermark_spawn_alpha;
    if (spawn_alpha <= 0.01f)
        return;

    try
    {
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();

        int ping = 0;
        const auto local = systems::g_local.get();
        if (local.controller && memory::is_game_ptr(local.controller))
            ping = static_cast<int>(reinterpret_cast<CCSPlayerController*>(local.controller)->m_iPing());

        const std::time_t now = std::time(nullptr);
        std::tm time_info{};
        localtime_s(&time_info, &now);

        ImFont* text_font = font->get(main_font_data, 13.0f);
        if (!text_font)
            return;

        char watermark_text[128]{};
        std::snprintf(
            watermark_text,
            sizeof(watermark_text),
            "kitty.cc | %d | %dms | %02d:%02d:%02d",
            static_cast<int>(io.Framerate),
            ping,
            time_info.tm_hour,
            time_info.tm_min,
            time_info.tm_sec);

        const ImVec2 text_size = text_font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, watermark_text);
        ImVec2 pos(watermark_pos.x, watermark_pos.y);
        ImRect watermark_rect(pos, ImVec2(pos.x + text_size.x, pos.y + text_size.y));

        const bool menu_open = var->gui.menu_open;
        const ImVec2 mouse_pos = ImGui::GetMousePos();

        if (menu_open && watermark_rect.Contains(mouse_pos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            is_dragging = true;
            drag_offset = ImVec2(mouse_pos.x - pos.x, mouse_pos.y - pos.y);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            is_dragging = false;

        if (menu_open && is_dragging)
        {
            watermark_pos.x = ImClamp(mouse_pos.x - drag_offset.x, 0.0f, io.DisplaySize.x - text_size.x);
            watermark_pos.y = ImClamp(mouse_pos.y - drag_offset.y, 0.0f, io.DisplaySize.y - text_size.y);
            pos = ImVec2(watermark_pos.x, watermark_pos.y);
        }

        if (menu_open)
        {
            const float handle_size = SCALE(8.0f);
            draw_list->AddRectFilled(
                ImVec2(pos.x, pos.y - handle_size - SCALE(4.0f)),
                ImVec2(pos.x + handle_size, pos.y - SCALE(4.0f)),
                IM_COL32(255, 255, 255, static_cast<int>(255 * spawn_alpha)));
        }

        draw_list->AddText(text_font, 13.0f, pos, IM_COL32(255, 255, 255, static_cast<int>(255 * spawn_alpha)), watermark_text);
    }
    catch (...)
    {
        is_dragging = false;
        dragging_item = -1;
    }
}
void c_watermark::cleanup()
{
    if (vertex_shader) { vertex_shader->Release(); vertex_shader = nullptr; }
    if (pixel_shader) { pixel_shader->Release(); pixel_shader = nullptr; }
    if (constant_buffer) { constant_buffer->Release(); constant_buffer = nullptr; }
    if (sampler_state) { sampler_state->Release(); sampler_state = nullptr; }
    if (blend_state) { blend_state->Release(); blend_state = nullptr; }
    
    initialized = false;
}


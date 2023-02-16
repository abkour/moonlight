#pragma once
#include "../../ext/d3dx12.h"
#include "../helpers.h"
#include "../project_defines.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>

namespace moonlight
{

void transition_resource(
    ID3D12GraphicsCommandList* command_list,
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before_state,
    D3D12_RESOURCE_STATES after_state
);

struct RenderTexture
{
    RenderTexture(DXGI_FORMAT format);

    void init(uint16_t window_width, uint16_t window_height);
    void set_device(ID3D12Device2* device, D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor, D3D12_CPU_DESCRIPTOR_HANDLE srv_descriptor);

    void transition(ID3D12GraphicsCommandList* command_list, D3D12_RESOURCE_STATES after_state);
    void transition_to_write_state(ID3D12GraphicsCommandList* command_list);
    void transition_to_read_state(ID3D12GraphicsCommandList* command_list);

    // Clears the texture with the color specified by the set_clear_color() functioon.
    void clear(ID3D12GraphicsCommandList* command_list);

    float* get_clear_color() 
    {
        return clear_color;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE get_rtv_descriptor() const
    {
        return rtv_descriptor;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE get_srv_descriptor() const
    {
        return srv_descriptor;
    }

    ID3D12Resource* get_resource()
    {
        return resource.Get();
    }

    D3D12_RESOURCE_STATES get_state() const
    {
        return resource_state;
    }

    void set_clear_color(DirectX::XMFLOAT4 clear_color)
    { 
        memcpy(this->clear_color, &clear_color.x, sizeof(float) * 4);
    }

private:

    Microsoft::WRL::ComPtr<ID3D12Device2> device;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    DXGI_FORMAT format;
    float clear_color[4];
    D3D12_RESOURCE_STATES resource_state;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor;
    D3D12_CPU_DESCRIPTOR_HANDLE srv_descriptor;
};

}
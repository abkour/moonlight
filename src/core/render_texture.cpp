#include "render_texture.hpp"

using namespace Microsoft::WRL;

namespace moonlight
{

template<typename T>
T* temp_address(T&& rvalue)
{
    return &rvalue;
}

RenderTexture::RenderTexture(DXGI_FORMAT format)
    : format(format)
    , resource_state(D3D12_RESOURCE_STATE_COMMON)
    , rtv_descriptor{}
    , srv_descriptor{}
{
}

void RenderTexture::set_device(
    ID3D12Device2* device,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor,
    D3D12_CPU_DESCRIPTOR_HANDLE srv_descriptor)
{
    this->device = device;
    this->rtv_descriptor = rtv_descriptor;
    this->srv_descriptor = srv_descriptor;
}

void RenderTexture::init(uint16_t window_width, uint16_t window_height)
{
    D3D12_RESOURCE_DESC rsc_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        window_width,
        window_height,
        1, 1, 1, 0, 
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    D3D12_CLEAR_VALUE clear_value = { format, {} };
    memcpy(clear_value.Color, clear_color, sizeof(clear_value.Color));

    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &rsc_desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clear_value,
        IID_PPV_ARGS(&resource)
    ));
    resource_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    device->CreateRenderTargetView(
        resource.Get(),
        nullptr,
        rtv_descriptor
    );

    device->CreateShaderResourceView(
        resource.Get(),
        nullptr,
        srv_descriptor
    );
}

void RenderTexture::transition(
    ID3D12GraphicsCommandList* command_list, 
    D3D12_RESOURCE_STATES after_state)
{
    transition_resource(command_list, resource.Get(), resource_state, after_state);
    resource_state = after_state;
}

void RenderTexture::transition_to_write_state(ID3D12GraphicsCommandList* command_list)
{
    transition(command_list, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RenderTexture::transition_to_read_state(ID3D12GraphicsCommandList* command_list)
{
    transition(command_list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void RenderTexture::clear(ID3D12GraphicsCommandList* command_list)
{
    command_list->ClearRenderTargetView(
        rtv_descriptor,
        clear_color,
        0,
        NULL
    );
}

}
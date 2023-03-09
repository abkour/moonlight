#include "render_texture.hpp"

using namespace Microsoft::WRL;

namespace moonlight
{

RenderTexture::RenderTexture(DXGI_FORMAT format)
    : m_format(format)
    , m_resource_state(D3D12_RESOURCE_STATE_COMMON)
    , m_rtv_descriptor{}
    , m_srv_descriptor{}
{
}

void RenderTexture::set_device(
    ID3D12Device2* device,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor,
    D3D12_CPU_DESCRIPTOR_HANDLE srv_descriptor)
{
    m_device = device;
    m_rtv_descriptor = rtv_descriptor;
    m_srv_descriptor = srv_descriptor;
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

    D3D12_CLEAR_VALUE clear_value = { m_format, {} };
    memcpy(clear_value.Color, m_clear_color, sizeof(clear_value.Color));

    ThrowIfFailed(m_device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &rsc_desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clear_value,
        IID_PPV_ARGS(&m_resource)
    ));
    m_resource_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    m_device->CreateRenderTargetView(
        m_resource.Get(),
        nullptr,
        m_rtv_descriptor
    );

    m_device->CreateShaderResourceView(
        m_resource.Get(),
        nullptr,
        m_srv_descriptor
    );
}

void RenderTexture::transition(
    ID3D12GraphicsCommandList* command_list, 
    D3D12_RESOURCE_STATES after_state)
{
    transition_resource(command_list, m_resource.Get(), m_resource_state, after_state);
    m_resource_state = after_state;
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
        m_rtv_descriptor,
        m_clear_color,
        0,
        NULL
    );
}

}
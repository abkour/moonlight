#include "cpu_gpu_texture2d.hpp"

namespace moonlight
{

CPUGPUTexture2D::CPUGPUTexture2D(
    ID3D12Resource* texture,
    DXGI_FORMAT format)
{
    m_texture = texture;
    m_format  = format;
    m_state   = D3D12_RESOURCE_STATE_COMMON;
}

void CPUGPUTexture2D::initialize(
    ID3D12Device2* device,
    uint32_t texture_width,
    uint32_t texture_height,
    uint32_t format_size)
{
    initialize_upload_texture(
        device,
        texture_width * texture_height * format_size
    );
}

void CPUGPUTexture2D::upload(
    ID3D12Device2* device,
    ID3D12GraphicsCommandList* command_list,
    D3D12_RESOURCE_STATES resource_state,
    void* data,
    unsigned width, unsigned height, unsigned format_size)
{
    D3D12_SUBRESOURCE_FOOTPRINT pitched_desc = {};
    pitched_desc.Format = m_format;
    pitched_desc.Width = width;
    pitched_desc.Height = height;
    pitched_desc.Depth = 1;
    pitched_desc.RowPitch = Align(width * format_size, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

    void* mapped_data;
    CD3DX12_RANGE read_range(0, 0);
    m_upload_texture->Map(0, &read_range, &mapped_data);
    m_data_cur = m_data_begin = reinterpret_cast<UINT8*>(mapped_data);
    m_data_end = m_data_begin + (width * height * format_size);

    suballocate_from_buffer(
        pitched_desc.Height * pitched_desc.RowPitch,
        D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT
    );

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_texture2D = {};
    placed_texture2D.Offset = m_data_cur - m_data_begin; // Offset to valid data
    placed_texture2D.Footprint = pitched_desc;

    for (UINT y = 0; y < height; ++y)
    {
        UINT8* data_u8 = reinterpret_cast<UINT8*>(data);
        UINT8* scanline = m_data_begin + placed_texture2D.Offset + y * pitched_desc.RowPitch;
        memcpy(scanline, data_u8 + y * width * format_size, format_size * width);
    }

    // Unmap after finished copying data into upload heap
    m_upload_texture->Unmap(0, nullptr);

    command_list->CopyTextureRegion(
        temp_address(CD3DX12_TEXTURE_COPY_LOCATION(m_texture.Get(), 0)),
        0, 0, 0,
        temp_address(CD3DX12_TEXTURE_COPY_LOCATION(m_upload_texture.Get(), placed_texture2D)),
        nullptr
    );
}

void CPUGPUTexture2D::resize(
    ID3D12Device2* device,
    ID3D12Resource* resized_texture,
    unsigned width, unsigned height, unsigned format_size)
{
    m_texture.Reset();
    m_texture = resized_texture;
    initialize_upload_texture(device, width * height * format_size);
}

void CPUGPUTexture2D::initialize_upload_texture(
    ID3D12Device2* device,
    size_t size)
{
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(size)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_upload_texture)
    ));
}

HRESULT CPUGPUTexture2D::suballocate_from_buffer(SIZE_T size, UINT align)
{
    m_data_cur = reinterpret_cast<UINT8*>(
        Align(reinterpret_cast<std::size_t>(m_data_cur), (std::size_t)align)
    );

    return (m_data_cur + size > m_data_end) ? E_INVALIDARG : S_OK;
}


}
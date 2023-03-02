#include "texture2D.hpp"

using namespace Microsoft::WRL;

namespace moonlight
{

Texture2D::Texture2D(
    ID3D12Device2* device, 
    ID3D12GraphicsCommandList* command_list,
    DXGI_FORMAT format, 
    void* data, unsigned width, unsigned height, unsigned format_size)
{
    D3D12_SUBRESOURCE_FOOTPRINT pitched_desc = {};
    pitched_desc.Format = format;
    pitched_desc.Width = width;
    pitched_desc.Height = height;
    pitched_desc.Depth = 1;
    pitched_desc.RowPitch = Align(width * format_size, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

    initialize_upload_buffer(device, pitched_desc.RowPitch * height);

    suballocate_from_buffer(
        pitched_desc.Height * pitched_desc.RowPitch,
        D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT
    );

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_texture2D = {};
    placed_texture2D.Offset = m_data_cur - m_data_begin;
    placed_texture2D.Footprint = pitched_desc;

    for (UINT y = 0; y < height; ++y)
    {
        UINT8* data_u8 = reinterpret_cast<UINT8*>(data);
        UINT8* scanline = m_data_begin + placed_texture2D.Offset + y * pitched_desc.RowPitch;
        memcpy(scanline, &(data_u8[y * width]), format_size * width);
    }

    D3D12_RESOURCE_DESC rsc_desc = {};
    rsc_desc.DepthOrArraySize = 1;
    rsc_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rsc_desc.Format = format;
    rsc_desc.MipLevels = 1;
    rsc_desc.Width = width;
    rsc_desc.Height = height;
    rsc_desc.SampleDesc = { 1, 0 };

    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &rsc_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_texture)
    ));

    command_list->CopyTextureRegion(
        temp_address(CD3DX12_TEXTURE_COPY_LOCATION(m_texture.Get(), 0)),
        0, 0, 0,
        temp_address(CD3DX12_TEXTURE_COPY_LOCATION(m_upload_buffer.Get(), placed_texture2D)),
        nullptr
    );
}

ID3D12Resource* Texture2D::get_underlying()
{
    return m_texture.Get();
}

void Texture2D::initialize_upload_buffer(ID3D12Device2* device, SIZE_T size)
{
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(size)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_upload_buffer)
    ));

    void* data;
    CD3DX12_RANGE read_range(0, 0);
    m_upload_buffer->Map(0, &read_range, &data);
    m_data_cur = m_data_begin = reinterpret_cast<UINT8*>(data);
    m_data_end = m_data_begin + size;
}

HRESULT Texture2D::suballocate_from_buffer(SIZE_T size, UINT align)
{
    m_data_cur = reinterpret_cast<UINT8*>(
        Align(reinterpret_cast<std::size_t>(m_data_cur), (std::size_t)align)
    );

    return (m_data_cur + size > m_data_end) ? E_INVALIDARG : S_OK;
}

}
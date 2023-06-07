#include "vertex_buffer_resource.hpp"

namespace moonlight
{

void VertexBufferResource::upload(
    ID3D12Device2* device,
    ID3D12GraphicsCommandList* command_list,
    std::size_t num_vertices,
    std::size_t stride,
    std::size_t sizeof_data_type,   // e.g 4 for float
    void* data)
{
    m_vertex_buffer = std::make_unique<DX12Resource>();
    m_vertex_buffer->upload(
        device,
        command_list,
        data,
        num_vertices * stride * sizeof_data_type,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

    m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
    m_vertex_buffer_view.SizeInBytes = num_vertices * stride * sizeof_data_type;
    m_vertex_buffer_view.StrideInBytes = stride * sizeof_data_type;
}

}
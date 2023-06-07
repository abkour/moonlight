#pragma once
#include <d3d12.h>
#include "dx12_resource.hpp"

namespace moonlight
{

class VertexBufferResource
{
public:

    VertexBufferResource() = default;

    void upload(
        ID3D12Device2* device,
        ID3D12GraphicsCommandList* command_list,
        std::size_t n_vertices, 
        std::size_t stride, 
        std::size_t sizeof_data_type,   // e.g 4 for float
        void* data
    );

    D3D12_VERTEX_BUFFER_VIEW& get_view()
    {
        return m_vertex_buffer_view;
    }

private:

    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        m_vertex_buffer_view;
};

}
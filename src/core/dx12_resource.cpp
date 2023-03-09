#include "dx12_resource.hpp"

using namespace Microsoft::WRL;

namespace moonlight
{

template<typename T>
T* temp_address(T&& rvalue)
{
    return &rvalue;
}

DX12Resource::DX12Resource()
    : m_state(D3D12_RESOURCE_STATE_COMMON)
{
}

DX12Resource::~DX12Resource()
{
}

void DX12Resource::transition(
    ID3D12GraphicsCommandList* command_list, 
    D3D12_RESOURCE_STATES new_state)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_resource.Get(),
        m_state,
        new_state
    );

    command_list->ResourceBarrier(1, &barrier);

    m_state = new_state;
}

void DX12Resource::update(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* command_list,
    void* data,
    size_t size_in_bytes)
{
    // the DEFAULT_HEAP resource needs to be in the COPY_DEST state, before it can be copied into.
    transition(command_list, D3D12_RESOURCE_STATE_COPY_DEST);

    // Copy the new data into the UPLOAD_HEAP resource
    UINT32* data_begin = nullptr;
    CD3DX12_RANGE read_range(0, 0);
    ThrowIfFailed(m_intermediate_buffer->Map(
        0,
        &read_range,
        reinterpret_cast<void**>(&data_begin)
    ));
    memcpy(data_begin, data, size_in_bytes);
    m_intermediate_buffer->Unmap(0, nullptr);

    // Memory layout description for the new data.
    D3D12_SUBRESOURCE_DATA data_desc = {};
    data_desc.pData = data;
    data_desc.RowPitch = size_in_bytes;
    data_desc.SlicePitch = size_in_bytes;
    
    // Upload the DEFAULT_HEAP resource with the contents of the UPLOAD_HEAP resource.
    UpdateSubresources(command_list, m_resource.Get(), m_intermediate_buffer.Get(), 0, 0, 1, &data_desc);

    // Finally, transition the resource into the VCB state
    transition(command_list, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void DX12Resource::upload(
    ID3D12Device2* device,
    ID3D12GraphicsCommandList* command_list,
    void* data,
    size_t size_in_bytes)
{
    m_state = D3D12_RESOURCE_STATE_COPY_DEST;
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    ));

    // Don't forget to release the intermediate buffer, once the data has been uploaded
    // to the GPU.
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_intermediate_buffer)
    ));

    D3D12_SUBRESOURCE_DATA data_desc = {};
    data_desc.pData = data;
    data_desc.RowPitch = size_in_bytes;
    data_desc.SlicePitch = size_in_bytes;

    UpdateSubresources(command_list, m_resource.Get(), m_intermediate_buffer.Get(), 0, 0, 1, &data_desc);

    // Create the CBV view
    transition(command_list, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}


}
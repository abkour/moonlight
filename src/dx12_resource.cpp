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
    : state(D3D12_RESOURCE_STATE_COMMON)
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
        resource.Get(),
        state,
        new_state
    );

    command_list->ResourceBarrier(1, &barrier);

    state = new_state;
}

void DX12Resource::upload(
    ID3D12Device2* device,
    ID3D12GraphicsCommandList* command_list,
    void* data,
    size_t size_in_bytes)
{
    state = D3D12_RESOURCE_STATE_COPY_DEST;
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource)
    ));

    // Don't forget to release the intermediate buffer, once the data has been uploaded
    // to the GPU.
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&intermediate_buffer)
    ));

    D3D12_SUBRESOURCE_DATA data_desc = {};
    data_desc.pData = data;
    data_desc.RowPitch = size_in_bytes;
    data_desc.SlicePitch = size_in_bytes;

    UpdateSubresources(command_list, resource.Get(), intermediate_buffer.Get(), 0, 0, 1, &data_desc);

    // Create the CBV view
    transition(
        command_list,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );
}


}
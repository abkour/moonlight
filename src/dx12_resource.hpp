#pragma once
#include "../ext/d3dx12.h"
#include "helpers.h"
#include "project_defines.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>

namespace moonlight
{

struct DX12Resource
{
    DX12Resource();
    ~DX12Resource();

    D3D12_RESOURCE_STATES get_state() const
    {
        return state;
    }

    ID3D12Resource* get_underlying()
    {
        return resource.Get();
    }

    D3D12_GPU_VIRTUAL_ADDRESS gpu_virtual_address()
    {
        return resource->GetGPUVirtualAddress();
    }

    void update(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* command_list,
        void* data,
        size_t size_in_bytes
    );

    void upload(
        ID3D12Device2* device,
        ID3D12GraphicsCommandList* command_list,
        void* data,
        size_t size_in_bytes
    );

    void transition(
        ID3D12GraphicsCommandList* command_list, 
        D3D12_RESOURCE_STATES new_state
    );

private:

    /*   Keeping track of the state is much more difficult than this.
    *    The state transition only happens after the responsible command list
    *    has been executed.
    */
    D3D12_RESOURCE_STATES state;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    // NOTE: This buffer is only a temporary solution. It is required, because
    // the buffer cannot be released before the data has been copied to the resource buffer.
    // Another solution would be to pass the intermediate buffer as argument. However,
    // that exposes more implementation details to the user. 
    // The goal for now is ease of use.
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediate_buffer;
};

}
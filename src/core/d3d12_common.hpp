#pragma once
#include <d3d12.h>
#include "../../ext/d3dx12.h"

namespace moonlight
{

inline void transition_resource(
    ID3D12GraphicsCommandList* command_list,
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before_state,
    D3D12_RESOURCE_STATES after_state)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource,
        before_state,
        after_state
    );

    command_list->ResourceBarrier(1, &barrier);
}

}
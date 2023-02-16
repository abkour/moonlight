#pragma once
#include "../../ext/d3dx12.h"
#include "../helpers.h"
#include "../project_defines.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace moonlight
{

class DescriptorHeap
{
public:

    DescriptorHeap(
        ID3D12Device2* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        UINT n_descriptors
    );

    ID3D12DescriptorHeap* get_underlying()
    {
        return m_descriptor_heap.Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(INT offset = 0);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(INT offset = 0);

private:

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_handle;
    SIZE_T m_handle_size;
};

}
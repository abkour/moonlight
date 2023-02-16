#include "descriptor_heap.hpp"

namespace moonlight
{

DescriptorHeap::DescriptorHeap(
    ID3D12Device2* device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    UINT n_descriptors)
{
    m_handle_size = device->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_FLAGS flags =
        (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
        : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.Flags = flags;
    dsv_heap_desc.NumDescriptors = n_descriptors;
    dsv_heap_desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(
        &dsv_heap_desc,
        IID_PPV_ARGS(&m_descriptor_heap)
    ));

    m_cpu_handle = m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_handle = m_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpu_handle(INT offset)
{
    return { SIZE_T(INT64(m_cpu_handle.ptr) + INT64(offset * m_handle_size)) };
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::gpu_handle(INT offset)
{
    return { SIZE_T(INT64(m_gpu_handle.ptr) + INT64(offset * m_handle_size)) };
}

}
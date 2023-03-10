#include "swap_chain.hpp"

namespace moonlight
{

using namespace Microsoft::WRL;

SwapChain::SwapChain(
    ID3D12Device2* device,
    ID3D12CommandQueue* command_queue,
    const uint16_t window_width, const uint16_t window_height,
    const HWND window_handle)
{
    ComPtr<IDXGIFactory4> factory4;
    UINT factory_flags = 0;
#ifdef _DEBUG
    factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = window_width;
    swap_chain_desc.Height = window_height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.Stereo = FALSE;
    swap_chain_desc.SampleDesc = { 1, 0 };
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 3;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags = 0;

    ComPtr<IDXGISwapChain1> swap_chain1;
    factory4->CreateSwapChainForHwnd(
        command_queue,
        window_handle,
        &swap_chain_desc,
        NULL,
        NULL,
        &swap_chain1
    );

    ThrowIfFailed(factory4->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER));

    swap_chain1.As(&m_swap_chain);

    m_rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_num_backbuffers
    );

    create_backbuffers(device);
}

void SwapChain::create_backbuffers(ID3D12Device2* device)
{
    for (uint8_t i = 0; i < m_num_backbuffers; ++i)
    {
        auto rtv_handle = m_rtv_descriptor_heap->cpu_handle(i);

        ComPtr<ID3D12Resource> backbuffer;
        m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffer));

        device->CreateRenderTargetView(
            backbuffer.Get(),
            nullptr,
            rtv_handle
        );

        m_backbuffers[i] = backbuffer;
    }
}

void SwapChain::transition_to_present(ID3D12GraphicsCommandList* command_list) 
{
    unsigned bb_idx = m_swap_chain->GetCurrentBackBufferIndex();
    transition_resource(
        command_list, m_backbuffers[bb_idx].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, 
        D3D12_RESOURCE_STATE_PRESENT
    );
}

void SwapChain::transition_to_rtv(ID3D12GraphicsCommandList* command_list)
{
    unsigned bb_idx = m_swap_chain->GetCurrentBackBufferIndex();
    transition_resource(
        command_list, m_backbuffers[bb_idx].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
}

void SwapChain::present()
{
    m_swap_chain->Present(1, 0);
}

void SwapChain::resize(ID3D12Device2* device, uint32_t width, uint32_t height)
{
    for (int i = 0; i < m_num_backbuffers; ++i)
    {
        m_backbuffers[i].Reset();
    }

    ThrowIfFailed(m_swap_chain->ResizeBuffers(
        m_num_backbuffers,
        width, height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0
    ));

    create_backbuffers(device);
}

uint8_t SwapChain::current_backbuffer_index()
{
    return m_swap_chain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::backbuffer_rtv_descriptor_handle(
    uint8_t backbuffer_idx)
{
    return m_rtv_descriptor_heap->cpu_handle(backbuffer_idx);
}

}
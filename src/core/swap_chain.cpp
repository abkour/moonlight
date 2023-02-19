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

    swap_chain1.As(&swap_chain);

    rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, n_backbuffers
    );

    create_backbuffers(device);
}

void SwapChain::create_backbuffers(ID3D12Device2* device)
{
    for (uint8_t i = 0; i < n_backbuffers; ++i)
    {
        auto rtv_handle = rtv_descriptor_heap->cpu_handle(i);

        ComPtr<ID3D12Resource> backbuffer;
        swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffer));

        device->CreateRenderTargetView(
            backbuffer.Get(),
            nullptr,
            rtv_handle
        );

        backbuffers[i] = backbuffer;
    }
}

void SwapChain::transition_to_present(ID3D12GraphicsCommandList* command_list) 
{
    unsigned bb_idx = swap_chain->GetCurrentBackBufferIndex();
    transition_resource(
        command_list, backbuffers[bb_idx].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, 
        D3D12_RESOURCE_STATE_PRESENT
    );
}

void SwapChain::transition_to_rtv(ID3D12GraphicsCommandList* command_list)
{
    unsigned bb_idx = swap_chain->GetCurrentBackBufferIndex();
    transition_resource(
        command_list, backbuffers[bb_idx].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
}

void SwapChain::present()
{
    swap_chain->Present(1, 0);
}

uint8_t SwapChain::current_backbuffer_index()
{
    return swap_chain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::backbuffer_rtv_descriptor_handle(
    uint8_t backbuffer_idx)
{
    return rtv_descriptor_heap->cpu_handle(backbuffer_idx);
}

}
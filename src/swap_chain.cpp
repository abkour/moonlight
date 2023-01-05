#include "swap_chain.hpp"

using namespace Microsoft::WRL;

namespace moonlight {

SwapChain::SwapChain(
	std::unique_ptr<Window>& window, 
	ComPtr<ID3D12CommandQueue> command_queue,
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12DescriptorHeap> descriptor_heap)
{
	initialize_dx12_swap_chain(window, command_queue);
	initialize_dx12_backbuffers(device, descriptor_heap);
}

void SwapChain::present(bool vsync, bool tearing) 
{
	UINT sync_interval = vsync ? 1 : 0;
	UINT present_flags = tearing && !vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	present_flags = 0;
	ThrowIfFailed(swap_chain->Present(sync_interval, present_flags));
}

void SwapChain::release_buffer(uint8_t buffer_idx)
{
	backbuffers[buffer_idx].Reset();
}

void SwapChain::resize_buffers(
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12DescriptorHeap> descriptor_heap,
	uint32_t width, 
	uint32_t height)
{
	constexpr uint8_t n_frames = 3;
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(swap_chain->GetDesc(&swapChainDesc));
	ThrowIfFailed(swap_chain->ResizeBuffers(
		n_frames, 
		width, 
		height,
		swapChainDesc.BufferDesc.Format,
		swapChainDesc.Flags));

	initialize_dx12_backbuffers(device, descriptor_heap);
}

uint8_t SwapChain::get_backbuffer_index()
{
	return swap_chain->GetCurrentBackBufferIndex();
}

ComPtr<ID3D12Resource> SwapChain::get_backbuffer(uint8_t index)
{
	return backbuffers[index];
}

void SwapChain::initialize_dx12_swap_chain(
	std::unique_ptr<Window>& window, 
	ComPtr<ID3D12CommandQueue> command_queue)
{
	constexpr uint8_t n_buffers = 3;
	ComPtr<IDXGIFactory4> factory4;
	UINT factory_flags = 0;
#ifdef _DEBUG
	factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // ???
	desc.BufferCount = n_buffers;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ???
	desc.Flags = window->tearing_supported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Height = window->height();
	desc.SampleDesc = { 1, 0 }; // ???
	desc.Scaling = DXGI_SCALING_STRETCH; // ???
	desc.Stereo = FALSE; // VR related flag
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // only flip-mode supported in dx12
	desc.Width = window->width();

	ComPtr<IDXGISwapChain1> swap_chain1;
	ThrowIfFailed(factory4->CreateSwapChainForHwnd(
		command_queue.Get(),
		window->handle,
		&desc,
		NULL,
		NULL,
		&swap_chain1
	));

	// MWA: MakeWindowAssociation
	ThrowIfFailed(factory4->MakeWindowAssociation(window->handle, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swap_chain1.As(&swap_chain));
}

void SwapChain::initialize_dx12_backbuffers(
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12DescriptorHeap> descriptor_heap)
{
	auto rtv_desc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(descriptor_heap->GetCPUDescriptorHandleForHeapStart());

	constexpr uint8_t n_buffers = 3;
	for (int i = 0; i < n_buffers; ++i) {
		ComPtr<ID3D12Resource> backbuffer;
		ThrowIfFailed(swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

		device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtv_handle);
		backbuffers[i] = backbuffer;

		rtv_handle.Offset(rtv_desc_size);
	}
}

}
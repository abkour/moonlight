#include "rendering_system.hpp"
#include "helpers.h"

namespace moonlight {

using namespace Microsoft::WRL;

RenderingSystem::RenderingSystem(std::unique_ptr<Window>& window, uint8_t number_of_backbuffers)
{
	system_initialized = false;
	D3D12_COMMAND_LIST_TYPE command_list_type =
		D3D12_COMMAND_LIST_TYPE_DIRECT;

	auto most_suitable_adapter = get_adapter();
	initialize_device(most_suitable_adapter);
	
	command_queue = std::make_unique<CommandQueue>(
		device,
		command_list_type);

	initialize_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, number_of_backbuffers);

	swap_chain = std::make_unique<SwapChain>(
		window,
		command_queue->get_underlying(),
		device,
		descriptor_heap);
	
	initialize_command_allocators(command_list_type);
	initialize_command_list(command_list_type);

	system_initialized = true;
}

/***************************************************************************/
// Methods for receiving underlying DX12 objects
ComPtr<ID3D12Device2> RenderingSystem::get_device() 
{
	return device;
}

ComPtr<ID3D12DescriptorHeap> RenderingSystem::get_descriptor_heap() 
{
	return descriptor_heap;
}

ComPtr<ID3D12CommandAllocator> RenderingSystem::get_command_allocator(const uint8_t idx) 
{
	return command_allocators[idx];
}

ComPtr<ID3D12GraphicsCommandList> RenderingSystem::get_command_list() 
{
	return command_list;
}

/***************************************************************************/
// Methods for receiving underlying wrapped DX12 objects
std::unique_ptr<CommandQueue>& RenderingSystem::get_command_queue()
{
	return command_queue;
}

std::unique_ptr<SwapChain>& RenderingSystem::get_swapchain()
{
	return swap_chain;
}

/***************************************************************************/
// Methods for initializing DX12 objects
ComPtr<IDXGIAdapter4> RenderingSystem::get_adapter() 
{
	// We are going to choose the adapter with the most VRAM, since VRAM is a reasonable
	// correlator for GPU power.
	ComPtr<IDXGIFactory4> factory4;
	UINT factory_flags = 0;
#ifdef _DEBUG
	factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));

	ComPtr<IDXGIAdapter1> adapter1;
	ComPtr<IDXGIAdapter4> adapter4;

	SIZE_T max_dedicated_vram = 0;
	for (UINT i = 0; factory4->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC1 adapter_desc1;
		adapter1->GetDesc1(&adapter_desc1);

		if ((adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) 
		{
			if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), 
				D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
				if (adapter_desc1.DedicatedVideoMemory > max_dedicated_vram) 
				{
					max_dedicated_vram = adapter_desc1.DedicatedSystemMemory;
					ThrowIfFailed(adapter1.As(&adapter4));
				}
			}
 		}
	}

	return adapter4;
}

void RenderingSystem::initialize_device(ComPtr<IDXGIAdapter4> adapter) 
{
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

	// Enable debug messaging in debug mode
#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> info_queue;
	if (SUCCEEDED(device.As(&info_queue))) {
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID deny_ids[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		filter.DenyList.NumIDs = _countof(deny_ids);
		filter.DenyList.pIDList = deny_ids;

		ThrowIfFailed(info_queue->PushStorageFilter(&filter));
	}
#endif
}

void RenderingSystem::initialize_descriptor_heap(
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t descriptor_count)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = descriptor_count;
	desc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap)));
}

void RenderingSystem::initialize_command_allocators(D3D12_COMMAND_LIST_TYPE type)
{
	for (int i = 0; i < 3; ++i) {
		ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocators[i])));
	}
}

void RenderingSystem::initialize_command_list(D3D12_COMMAND_LIST_TYPE type)
{
	ThrowIfFailed(device->CreateCommandList(
		0, 
		type, 
		command_allocators[swap_chain->get_backbuffer_index()].Get(),
		nullptr, 
		IID_PPV_ARGS(&command_list)));
	 
	ThrowIfFailed(command_list->Close());
}

//
//
// Public API
void RenderingSystem::flush()
{
	uint8_t current_backbuffer_idx = swap_chain->get_backbuffer_index();
	command_queue->flush(current_backbuffer_idx);
}

/***************************************************************************/
// Public API
void RenderingSystem::render(std::unique_ptr<Window>& window)
{
	uint8_t current_backbuffer_idx = swap_chain->get_backbuffer_index();
	auto command_allocator = command_allocators[current_backbuffer_idx];
	auto backbuffer = swap_chain->get_backbuffer(current_backbuffer_idx);

	command_allocator->Reset();
	command_list->Reset(command_allocator.Get(), nullptr);

	// Clear the render target
	{
		CD3DX12_RESOURCE_BARRIER barrier =
			CD3DX12_RESOURCE_BARRIER::Transition(
				backbuffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);

		command_list->ResourceBarrier(1, &barrier);

		FLOAT clear_color[] = { 1.f, 0.3f, 0.15f, 1.f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
			descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
			current_backbuffer_idx,
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	
		command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
	}

	// Present to the swap chain
	{
		CD3DX12_RESOURCE_BARRIER barrier =
			CD3DX12_RESOURCE_BARRIER::Transition(
				backbuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);

		command_list->ResourceBarrier(1, &barrier);

		ThrowIfFailed(command_list->Close());

		ID3D12CommandList* const command_lists[] =
		{
			command_list.Get()
		};
		
		command_queue->execute(command_lists, _countof(command_lists));

		BOOL tearing_supported = FALSE;
		swap_chain->present(window->is_vsync_on(), tearing_supported);

		command_queue->signal(current_backbuffer_idx);

		current_backbuffer_idx = swap_chain->get_backbuffer_index();

		command_queue->wait_for_finished(current_backbuffer_idx);
	}
}

void RenderingSystem::resize(std::unique_ptr<Window>& window)
{
	if (window->resize()) {
		command_queue->flush(swap_chain->get_backbuffer_index());
		
		for (uint8_t i = 0; i < 3; ++i) {
			swap_chain->release_buffer(i);
			command_queue->reset_frame_fence_value(i, command_queue->get_fence_value());
		}
		
		swap_chain->resize_buffers(device, descriptor_heap,window->width(), window->height());
	}
}

}
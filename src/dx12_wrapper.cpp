#include "dx12_wrapper.hpp"

using namespace Microsoft::WRL;

namespace moonlight {

ComPtr<IDXGIAdapter4> DX12Wrapper::create_adapter()
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

ComPtr<ID3D12Device2> DX12Wrapper::create_device(
	ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> device;
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

	return device;
}

ComPtr<ID3D12DescriptorHeap> DX12Wrapper::create_descriptor_heap(
	ComPtr<ID3D12Device2> device,
	D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
	uint32_t descriptor_count)
{
	ComPtr<ID3D12DescriptorHeap> descriptor_heap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = descriptor_count;
	desc.Type = descriptor_type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap)));

	return descriptor_heap;
}

ComPtr<ID3D12CommandAllocator> DX12Wrapper::create_command_allocator(
	ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE command_list_type)
{
	ComPtr<ID3D12CommandAllocator> command_allocator;

	ThrowIfFailed(device->CreateCommandAllocator(command_list_type, IID_PPV_ARGS(&command_allocator)));
	
	return command_allocator;
}

ComPtr<ID3D12GraphicsCommandList2> DX12Wrapper::create_command_list(
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12CommandAllocator> command_allocator,
	D3D12_COMMAND_LIST_TYPE command_list_type)
{
	ComPtr<ID3D12GraphicsCommandList2> command_list;

	ThrowIfFailed(device->CreateCommandList(
		0,
		command_list_type,
		command_allocator.Get(),
		nullptr,
		IID_PPV_ARGS(&command_list)));

	ThrowIfFailed(command_list->Close());

	return command_list;
}

std::unique_ptr<SwapChain> DX12Wrapper::create_swap_chain(
	std::unique_ptr<Window>& window,
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12DescriptorHeap> descriptor_heap,
	std::unique_ptr<CommandQueue>& command_queue)
{
	return std::make_unique<SwapChain>(
		window,
		command_queue->get_d3d12_command_queue(),
		device,
		descriptor_heap
	);
}

std::unique_ptr<CommandQueue> DX12Wrapper::create_command_queue(
	ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE command_list_type)
{
	return std::make_unique<CommandQueue>(
		device,
		command_list_type
	);
}

}
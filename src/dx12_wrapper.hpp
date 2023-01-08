#pragma once
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>

// Direct3D12 extension library
#include "../ext/d3dx12.h"


#include <wrl.h>

#include <cstdint>

#include "command_queue.hpp"
#include "swap_chain.hpp"
#include "window.hpp"

namespace moonlight {

class DX12Wrapper {

public:

	static Microsoft::WRL::ComPtr<IDXGIAdapter4> create_adapter();
	
	static Microsoft::WRL::ComPtr<ID3D12Device2> create_device(
		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter
	);

	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> create_descriptor_heap(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
		uint32_t descriptor_count
	);

	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> create_command_allocator(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);

	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> create_command_list(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);

	static std::unique_ptr<SwapChain> create_swap_chain(
		std::unique_ptr<Window>& window,
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap,
		std::unique_ptr<CommandQueue>& command_queue
	);

	static std::unique_ptr<CommandQueue> create_command_queue(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);
};

}
#pragma once
#include <Windows.h>
#include <shellapi.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// Direct3D12 extension library
#include "../ext/d3dx12.h"


#include <wrl.h>

#include <chrono>
#include <cstdint>

#include "command_queue.hpp"
#include "swap_chain.hpp"
#include "window.hpp"

namespace moonlight {

class RenderingSystem {

public:

	RenderingSystem(std::unique_ptr<Window>& window, uint8_t number_of_backbuffers);
	
	/***************************************************************************/
	// Methods for receiving underlying DX12 objects
	Microsoft::WRL::ComPtr<ID3D12Device2> get_device();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> get_descriptor_heap();
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> get_command_allocator(const uint8_t idx);
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> get_command_list();

	/***************************************************************************/
	// Methods for receiving underlying wrapped DX12 objects
	std::unique_ptr<CommandQueue>& get_command_queue();
	std::unique_ptr<SwapChain>& get_swapchain();

	/***************************************************************************/
	// General API
	void flush();
	void render(std::unique_ptr<Window>& window);
	void resize(std::unique_ptr<Window>& window);
	bool is_initialized() { 
		return system_initialized; 
	}

private:
	
	/* Implementaton details */
	
	// Device creation
	Microsoft::WRL::ComPtr<IDXGIAdapter4> get_adapter();
	void initialize_device(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);

	// Descriptor heap creation
	void initialize_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptor_count);

	void initialize_command_allocators(D3D12_COMMAND_LIST_TYPE type);

	void initialize_command_list(D3D12_COMMAND_LIST_TYPE type);

private:

	/* Objects */

	Microsoft::WRL::ComPtr<ID3D12Device2> device;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocators[3];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;

	std::unique_ptr<CommandQueue> command_queue;
	std::unique_ptr<SwapChain> swap_chain;

private:

	/* Variables */

	bool system_initialized; // This flag is set once rendering can take place.
};

}
#pragma once
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

// Direct3D12 extension library
#include "../ext/d3dx12.h"
#include "window.hpp"

#include <cstdint>
#include <memory>

namespace moonlight {

class SwapChain {

public:

	SwapChain(
		std::unique_ptr<Window>& window, 
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue,
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap);

	uint8_t get_backbuffer_index();
	
	Microsoft::WRL::ComPtr<ID3D12Resource> get_backbuffer(uint8_t index);

	void present(bool vsync, bool tearing);

	void release_buffer(uint8_t buffer_idx);

	void resize_buffers(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap,
		uint32_t width, 
		uint32_t height);

private:

	void initialize_dx12_swap_chain(
		std::unique_ptr<Window>& window, 
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue);

	void initialize_dx12_backbuffers(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap);

private:

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain;
	Microsoft::WRL::ComPtr<ID3D12Resource> backbuffers[3];
	
	UINT current_backbuffer_index;
};

}
#pragma once
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

// Direct3D12 extension library
#include "../ext/d3dx12.h"
#include "window.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>

namespace moonlight {

class CommandQueue {

public:

	CommandQueue(
		Microsoft::WRL::ComPtr<ID3D12Device2> device, 
		D3D12_COMMAND_LIST_TYPE type);

	~CommandQueue();

	void execute(ID3D12CommandList* const* command_list, std::size_t n_command_lists);

	// Flushes the GPU:
	void flush(uint8_t current_backbuffer_idx);

	void reset_frame_fence_value(uint8_t buffer_idx, uint64_t frame_fence_value);

	// Signal a fence value from the GPU
	void signal(uint8_t buffer_index);

	// wait for GPU
	void wait_for_finished(
		uint8_t buffer_index,
		std::chrono::milliseconds duration = std::chrono::milliseconds::max());

	uint64_t get_fence_value() const;

	// Retrieve the underlying dx12 object
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> get_underlying();

private:

	void initialize_dx12_command_queue(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE type);

	void initialize_dx12_fence(Microsoft::WRL::ComPtr<ID3D12Device2> device);

	void initialize_event();

private:

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	uint64_t fence_value;
	uint64_t frame_fence_values[3];
	HANDLE fence_event;
};

}
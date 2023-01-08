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
#include <queue>

namespace moonlight {

class CommandQueue {

public:

	CommandQueue(
		Microsoft::WRL::ComPtr<ID3D12Device2> device, 
		D3D12_COMMAND_LIST_TYPE type);

	~CommandQueue();

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> get_command_list();

	uint64_t execute_command_list(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list);

	uint64_t signal();

	bool is_fence_complete(uint64_t fence_value);

	void wait_for_fence_value(uint64_t fence_value);

	void flush();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> get_d3d12_command_queue() const;

protected:

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> create_command_allocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> create_command_list(
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator
	);

private:

	struct CommandAllocatorEntry {
		uint64_t fence_value;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	};

	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;

	D3D12_COMMAND_LIST_TYPE						command_list_type;
	Microsoft::WRL::ComPtr<ID3D12Device2>		d3d12_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	d3d12_command_queue;
	Microsoft::WRL::ComPtr<ID3D12Fence>			d3d12_fence;
	HANDLE										fence_event;
	uint64_t									fence_value;

	CommandAllocatorQueue						command_allocator_queue;
	CommandListQueue							command_list_queue;
};

}
#include "command_queue.hpp"

using namespace Microsoft::WRL;

namespace moonlight {

//
//
// Public API
CommandQueue::CommandQueue(
	ComPtr<ID3D12Device2> device, 
	D3D12_COMMAND_LIST_TYPE type)
{
	fence_value = 0;
	std::memset(frame_fence_values, 0, sizeof(frame_fence_values));

	initialize_dx12_command_queue(device, type);
	initialize_dx12_fence(device);
	initialize_event();
}

CommandQueue::~CommandQueue()
{
	::CloseHandle(fence_event);
}


void CommandQueue::execute(ID3D12CommandList* const* command_lists, std::size_t n_command_lists)
{
	command_queue->ExecuteCommandLists(n_command_lists, command_lists);
}

// Flushes the GPU
void CommandQueue::flush(uint8_t current_backbuffer_idx) 
{
	signal(current_backbuffer_idx);
	wait_for_finished(current_backbuffer_idx);
}

uint64_t CommandQueue::get_fence_value() const
{
	return fence_value;
}

ComPtr<ID3D12CommandQueue> CommandQueue::get_underlying()
{
	return command_queue;
}

void CommandQueue::reset_frame_fence_value(uint8_t buffer_idx, uint64_t frame_fence_value)
{
	frame_fence_values[buffer_idx] = frame_fence_value;
}

// Signal a fence value from the GPU
void CommandQueue::signal(uint8_t buffer_index) 
{
	uint64_t signal_value = ++fence_value;
	ThrowIfFailed(command_queue->Signal(fence.Get(), signal_value));

	frame_fence_values[buffer_index] = signal_value;
}

// wait for GPU
void CommandQueue::wait_for_finished(
	uint8_t buffer_index,
	std::chrono::milliseconds duration)
{
	uint64_t frame_fence_value = frame_fence_values[buffer_index];
	if (fence->GetCompletedValue() < frame_fence_value) {
		ThrowIfFailed(fence->SetEventOnCompletion(frame_fence_value, fence_event));
		::WaitForSingleObject(fence_event, static_cast<DWORD>(duration.count()));
	}
}

//
//
// Implementation
void CommandQueue::initialize_dx12_command_queue(
	ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.NodeMask = 0;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&command_queue)));
}

void CommandQueue::initialize_dx12_fence(
	ComPtr<ID3D12Device2> device)
{
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void CommandQueue::initialize_event()
{
	fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fence_event && "Failed to create fence event");
}

}
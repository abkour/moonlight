#include "command_queue.hpp"

using namespace Microsoft::WRL;

namespace moonlight {

CommandQueue::CommandQueue(
	ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE type)
	: fence_value(0)
	, command_list_type(type)
	, d3d12_device(device)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(d3d12_device->CreateCommandQueue(
		&desc, 
		IID_PPV_ARGS(&d3d12_command_queue))
	);

	ThrowIfFailed(d3d12_device->CreateFence(
		fence_value, 
		D3D12_FENCE_FLAG_NONE, 
		IID_PPV_ARGS(&d3d12_fence))
	);

	fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fence_event && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
}

ComPtr<ID3D12CommandAllocator> CommandQueue::create_command_allocator()
{
	ComPtr<ID3D12CommandAllocator> command_allocator;
	ThrowIfFailed(d3d12_device->CreateCommandAllocator(
		command_list_type, 
		IID_PPV_ARGS(&command_allocator))
	);

	return command_allocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::create_command_list(
	ComPtr<ID3D12CommandAllocator> allocator)
{
	ComPtr<ID3D12GraphicsCommandList2> command_list;
	ThrowIfFailed(d3d12_device->CreateCommandList(
		0, 
		command_list_type, 
		allocator.Get(), 
		nullptr, 
		IID_PPV_ARGS(&command_list))
	);

	return command_list;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::get_command_list()
{
	ComPtr<ID3D12CommandAllocator> command_allocator;
	ComPtr<ID3D12GraphicsCommandList2> command_list;

	if (!command_allocator_queue.empty() 
		&& is_fence_complete(command_allocator_queue.front().fence_value)) 
	{
		command_allocator = command_allocator_queue.front().command_allocator;
		command_allocator_queue.pop();

		ThrowIfFailed(command_allocator->Reset());
	} else 
	{
		command_allocator = create_command_allocator();
	}

	if (!command_list_queue.empty()) 
	{
		command_list = command_list_queue.front();
		command_list_queue.pop();

		ThrowIfFailed(command_list->Reset(command_allocator.Get(), NULL));
	} else 
	{
		command_list = create_command_list(command_allocator);
	}

	ThrowIfFailed(command_list->SetPrivateDataInterface(
		__uuidof(ID3D12CommandAllocator), 
		command_allocator.Get())
	);

	return command_list;
}

uint64_t CommandQueue::execute_command_list(
	ComPtr<ID3D12GraphicsCommandList2> command_list)
{
	command_list->Close();

	ID3D12CommandAllocator* command_allocator = nullptr;
	UINT data_size = sizeof(command_allocator);
	ThrowIfFailed(command_list->GetPrivateData(
		__uuidof(ID3D12CommandAllocator), 
		&data_size,
		&command_allocator)
	);

	ID3D12CommandList* const command_lists[] =
	{
		command_list.Get()
	};

	d3d12_command_queue->ExecuteCommandLists(1, command_lists);
	uint64_t fence_value = signal();
	
	command_allocator_queue.emplace(CommandAllocatorEntry{ fence_value, command_allocator });
	command_list_queue.push(command_list);

	command_allocator->Release();
	
	return fence_value;
}

uint64_t CommandQueue::signal()
{
	uint64_t fence_value = ++this->fence_value;
	d3d12_command_queue->Signal(d3d12_fence.Get(), fence_value);
	return fence_value;
}

bool CommandQueue::is_fence_complete(uint64_t fence_value)
{
	return d3d12_fence->GetCompletedValue() >= fence_value;
}

void CommandQueue::wait_for_fence_value(uint64_t fence_value)
{
	if (!is_fence_complete(fence_value))
	{
		d3d12_fence->SetEventOnCompletion(fence_value, fence_event);
		::WaitForSingleObject(fence_event, DWORD_MAX);
	}
}

void CommandQueue::flush()
{
	wait_for_fence_value(signal());
}

ComPtr<ID3D12CommandQueue> CommandQueue::get_d3d12_command_queue() const
{
	return d3d12_command_queue;
}

}
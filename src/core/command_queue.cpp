#include "command_queue.hpp"
#include "../helpers.h"

namespace moonlight
{

using namespace Microsoft::WRL;

CommandQueue::CommandQueue(ID3D12Device2* device)
    : fence_value(0)
    , fence_event(::CreateEvent(NULL, FALSE, FALSE, NULL))
{
    ThrowIfFailed(device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence)
    ));

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));
}

void CommandQueue::execute_command_list(
    ID3D12CommandList** command_lists, unsigned n_lists)
{
    command_queue->ExecuteCommandLists(n_lists, command_lists);
}


void CommandQueue::flush()
{
    signal();
    wait_for_fence();
}

uint64_t CommandQueue::signal()
{
    ++fence_value;
    ThrowIfFailed(command_queue->Signal(fence.Get(), fence_value));
    return fence_value;
}

void CommandQueue::wait_for_fence()
{
    if (fence->GetCompletedValue() < fence_value)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
        WaitForSingleObject(fence_event, INFINITE);
    }
}

}
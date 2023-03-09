#include "command_queue.hpp"
#include "../helpers.h"

namespace moonlight
{

using namespace Microsoft::WRL;

CommandQueue::CommandQueue(ID3D12Device2* device)
    : m_fence_value(0)
    , m_fence_event(::CreateEvent(NULL, FALSE, FALSE, NULL))
{
    ThrowIfFailed(device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&m_fence)
    ));

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_command_queue)));
}

void CommandQueue::execute_command_list(
    ID3D12CommandList** command_lists, unsigned n_lists)
{
    m_command_queue->ExecuteCommandLists(n_lists, command_lists);
}


void CommandQueue::flush()
{
    signal();
    wait_for_fence();
}

uint64_t CommandQueue::signal()
{
    ++m_fence_value;
    ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), m_fence_value));
    return m_fence_value;
}

void CommandQueue::wait_for_fence()
{
    if (m_fence->GetCompletedValue() < m_fence_value)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fence_value, m_fence_event));
        WaitForSingleObject(m_fence_event, INFINITE);
    }
}

}
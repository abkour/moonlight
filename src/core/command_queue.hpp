#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

namespace moonlight
{

class CommandQueue
{
public:

    CommandQueue(ID3D12Device2* devic);

    void        execute_command_list(ID3D12CommandList** command_lists, unsigned n_lists);
    void        flush();
    uint64_t    signal();
    void        wait_for_fence();

    ID3D12CommandQueue* get_underlying()
    {
        return command_queue.Get();
    }

    uint64_t get_fence_value() const
    {
        return  fence_value;
    }

private:

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    uint64_t fence_value;
    HANDLE fence_event;
};

}
#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "descriptor_heap.hpp"
#include "d3d12_common.hpp"

namespace moonlight
{

class SwapChain
{
public:

    SwapChain(
        ID3D12Device2* device,
        ID3D12CommandQueue* command_queue, 
        const uint16_t window_width, const uint16_t window_height,
        const HWND window_handle
    );

    void transition_to_present(ID3D12GraphicsCommandList* command_list);
    void transition_to_rtv(ID3D12GraphicsCommandList* command_list);
    void present();
    void resize(ID3D12Device2* device, uint32_t width, uint32_t height);

    uint8_t current_backbuffer_index();

    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_descriptor_handle(uint8_t backbuffer_idx);

    void take_screenshot(ID3D12GraphicsCommandList* command_list, const char* file_location);

    DXGI_FORMAT get_pixel_format() const
    {
        return m_pixel_format;
    };

    ID3D12Resource* get_backbuffer(uint8_t bb_index)
    {
        return m_backbuffers[bb_index].Get();
    }

private:

    void create_backbuffers(ID3D12Device2* device);

private:

    unsigned m_num_backbuffers = 3;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swap_chain;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_backbuffers[3];
    std::unique_ptr<DescriptorHeap> m_rtv_descriptor_heap;
    DXGI_FORMAT m_pixel_format;
    D3D12_RESOURCE_STATES m_resource_state;
};

}
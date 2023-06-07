#pragma once
#include "../ext/d3dx12.h"
#include "camera.hpp"
#include "logging_file.hpp"
#include "core/command_queue.hpp"
#include "core/descriptor_heap.hpp"
#include "core/global_pss_field.hpp"
#include "core/key_state.hpp"
#include "core/swap_chain.hpp"
#include "project_defines.hpp"
#include "window.hpp"

#include <bitset>
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <DirectXMath.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

namespace moonlight {

class IApplication {

public:

    IApplication() = delete;
    IApplication(HINSTANCE hinstance);
    IApplication(HINSTANCE hinstance, WNDPROC wndproc);

    virtual ~IApplication() 
    {
        CoUninitialize();
    }

    virtual void flush()
    {
        m_command_queue->flush();
    }

    virtual void on_key_event(const PackedKeyArguments key_state)
    {}

    virtual void on_mouse_move(LPARAM lparam);

    virtual void update() = 0;
    
    virtual void render() = 0;

    virtual void resize() = 0;

    virtual void clear_rtv_dsv(
        const DirectX::XMFLOAT4 clear_color = DirectX::XMFLOAT4(0.7f, 0.7f, 0.7f, 1.f));
    virtual void present();

    void run();

    static LRESULT CALLBACK WindowMessagingProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:

    virtual void initialize_raw_input_devices();

    virtual Microsoft::WRL::ComPtr<IDXGIAdapter4> _pimpl_create_adapter();

    virtual Microsoft::WRL::ComPtr<ID3D12Device2> _pimpl_create_device(
        Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter
    );

    virtual Microsoft::WRL::ComPtr<ID3D12CommandQueue> _pimpl_create_command_queue(
        Microsoft::WRL::ComPtr<ID3D12Device2> device
    );

    virtual Microsoft::WRL::ComPtr<IDXGISwapChain4> _pimpl_create_swap_chain(
        ID3D12CommandQueue* command_queue,
        const uint16_t window_width,
        const uint16_t window_height
    );

    virtual Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _pimpl_create_rtv_descriptor_heap(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        UINT n_descriptors
    );

    virtual Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _pimpl_create_dsv_descriptor_heap(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        UINT n_descriptors
    );

    virtual Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _pimpl_create_srv_descriptor_heap(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        UINT n_descriptors
    );

    virtual Microsoft::WRL::ComPtr<ID3D12Resource> _pimpl_create_dsv(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_descriptor,
        const uint16_t window_width,
        const uint16_t window_height
    );

    virtual void _pimpl_create_backbuffers(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain,
        ID3D12DescriptorHeap* descriptor_heap,
        Microsoft::WRL::ComPtr<ID3D12Resource>* backbuffers,
        uint8_t n_backbuffers
    );

    virtual Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _pimpl_create_command_allocator(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        D3D12_COMMAND_LIST_TYPE type
    );

    virtual Microsoft::WRL::ComPtr<ID3D12CommandList> _pimpl_create_command_list_copy(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
        D3D12_COMMAND_LIST_TYPE type
    );

    virtual Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _pimpl_create_command_list(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
        D3D12_COMMAND_LIST_TYPE type
    );

    virtual Microsoft::WRL::ComPtr<ID3D12Fence> _pimpl_create_fence(
        Microsoft::WRL::ComPtr<ID3D12Device2> device
    );

    HANDLE _pimpl_create_fence_event();

protected:

    Microsoft::WRL::ComPtr<ID3D12Device2>             m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_depth_buffer;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list_direct;

    std::unique_ptr<SwapChain>      m_swap_chain;
    std::unique_ptr<CommandQueue>   m_command_queue;
    std::unique_ptr<DescriptorHeap> m_dsv_descriptor_heap;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT     m_scissor_rect;

    Camera m_camera;
    DirectX::XMMATRIX m_mvp_matrix;

    std::shared_ptr<GlobalPipelineStateStreamField> m_shared_pss_field;

protected:

    bool m_application_initialized = false;
    std::unique_ptr<Window> m_window;
    KeyState m_keyboard_state;

private:
    
    void update_keystates(PackedKeyArguments key_state);

};

}
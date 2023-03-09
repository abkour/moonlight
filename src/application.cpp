#include "application.hpp"

template<typename T>
T* temp_address(T&& rvalue)
{
    return &rvalue;
}

using namespace Microsoft::WRL;

namespace moonlight {

IApplication::IApplication(HINSTANCE hinstance)
{
    ThrowIfFailed(CoInitializeEx(NULL, COINIT_MULTITHREADED));
}

void IApplication::run() 
{
    ::ShowWindow(m_window->handle, SW_SHOW);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    // Make sure the command queue has finished all commands before closing.
    flush();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK IApplication::WindowMessagingProcess(
    HWND hwnd, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam) 
{
    IApplication* app = nullptr;
    if (message == WM_NCCREATE) {
        // Before the window is created, we store the this pointer in the 
        // user_data field associated with the window.
        app = static_cast<IApplication*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        SetLastError(0);    // ???
        if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app))) {
            if (GetLastError() != 0) {
                return FALSE;
            }
        }
    }

    // Retrieve the pointer. app is nullptr, when WM_NCCREATE has not yet been handled.
    app = reinterpret_cast<IApplication*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (app == nullptr) {
        return FALSE;
    }
    
    if (app->is_application_initialized()) 
    {
        UINT msg = message;

        switch (message) {
        case WM_PAINT:
            app->update();
            app->render();
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            switch (wParam) {
            case 'V':
                app->m_window->flip_vsync();
                break;
            case VK_ESCAPE:
                ::PostQuitMessage(0);
                break;
            case VK_F11:
                app->m_window->flip_fullscreen();
                break;
            default:
            {
                KeyCode key = (KeyCode)wParam;
                PackedKeyArguments key_state(
                    key, KeyCode::Reserved, PackedKeyArguments::KeyState::Pressed
                );
                app->on_key_event(key_state);
            }
                break;
            }
        }
        break;
        case WM_INPUT:
            app->on_mouse_move(lParam);
            break;
        case WM_SYSKEYUP:
        case WM_KEYUP: 
        {
            KeyCode key = (KeyCode)wParam;
            PackedKeyArguments key_state(
                key, KeyCode::Reserved, PackedKeyArguments::KeyState::Released
            );
            app->on_key_event(key_state);
        }
            break;
        case WM_SYSCHAR:
            break;
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT)
            {
                SetCursor(NULL);
            }
            break;
        case WM_SIZE:
            app->resize();
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;
        default:
            return ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    } else { 
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

Microsoft::WRL::ComPtr<IDXGIAdapter4> IApplication::_pimpl_create_adapter()
{
    // We are going to choose the adapter with the most VRAM, since VRAM is a reasonable
    // correlator for GPU power.
    ComPtr<IDXGIFactory4> factory4;
    UINT factory_flags = 0;
#ifdef _DEBUG
    factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));

    ComPtr<IDXGIAdapter1> adapter1;
    ComPtr<IDXGIAdapter4> adapter4;

    SIZE_T max_dedicated_vram = 0;
    for (UINT i = 0; factory4->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 adapter_desc1;
        adapter1->GetDesc1(&adapter_desc1);

        if ((adapter_desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
        {
            if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(),
                D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
                if (adapter_desc1.DedicatedVideoMemory > max_dedicated_vram)
                {
                    max_dedicated_vram = adapter_desc1.DedicatedSystemMemory;
                    ThrowIfFailed(adapter1.As(&adapter4));
                }
            }
        }
    }

    return adapter4;
}

ComPtr<ID3D12Device2> IApplication::_pimpl_create_device(
    ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device2> device;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

    // Enable debug messaging in debug mode
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> info_queue;
    if (SUCCEEDED(device.As(&info_queue))) {
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        D3D12_MESSAGE_ID deny_ids[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        filter.DenyList.NumIDs = _countof(deny_ids);
        filter.DenyList.pIDList = deny_ids;

        ThrowIfFailed(info_queue->PushStorageFilter(&filter));
    }
#endif

    return device;
}

ComPtr<ID3D12CommandQueue> IApplication::_pimpl_create_command_queue(
    ComPtr<ID3D12Device2> device)
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ComPtr<ID3D12CommandQueue> command_queue;
    ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

    return command_queue;
}

ComPtr<IDXGISwapChain4> IApplication::_pimpl_create_swap_chain(
    ID3D12CommandQueue* command_queue,
    const uint16_t window_width,
    const uint16_t window_height)
{
    ComPtr<IDXGIFactory4> factory4;
    UINT factory_flags = 0;
#ifdef _DEBUG
    factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.Width = window_width;
    swap_chain_desc.Height = window_height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.Stereo = FALSE;
    swap_chain_desc.SampleDesc = { 1, 0 };
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 3;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc.Flags = 0;

    ComPtr<IDXGISwapChain1> swap_chain1;
    factory4->CreateSwapChainForHwnd(
        command_queue,
        m_window->handle,
        &swap_chain_desc,
        NULL,
        NULL,
        &swap_chain1
    );

    ThrowIfFailed(factory4->MakeWindowAssociation(m_window->handle, DXGI_MWA_NO_ALT_ENTER));

    ComPtr<IDXGISwapChain4> swap_chain;
    swap_chain1.As(&swap_chain);

    return swap_chain;
}

ComPtr<ID3D12DescriptorHeap> IApplication::_pimpl_create_rtv_descriptor_heap(
    ComPtr<ID3D12Device2> device,
    UINT n_descriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_heap_desc.NodeMask = 0;
    rtv_heap_desc.NumDescriptors = n_descriptors;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap;
    ThrowIfFailed(device->CreateDescriptorHeap(
        &rtv_heap_desc,
        IID_PPV_ARGS(&rtv_descriptor_heap)
    ));

    return rtv_descriptor_heap;
}

ComPtr<ID3D12DescriptorHeap> IApplication::_pimpl_create_dsv_descriptor_heap(
    ComPtr<ID3D12Device2> device,
    UINT n_descriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
    dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_heap_desc.NumDescriptors = n_descriptors;
    dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap;
    ThrowIfFailed(device->CreateDescriptorHeap(
        &dsv_heap_desc,
        IID_PPV_ARGS(&dsv_descriptor_heap)
    ));

    return dsv_descriptor_heap;
}

ComPtr<ID3D12DescriptorHeap> IApplication::_pimpl_create_srv_descriptor_heap(
    ComPtr<ID3D12Device2> device,
    UINT n_descriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
    srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srv_heap_desc.NumDescriptors = n_descriptors;
    srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ComPtr<ID3D12DescriptorHeap> srv_descriptor_heap;
    ThrowIfFailed(device->CreateDescriptorHeap(
        &srv_heap_desc,
        IID_PPV_ARGS(&srv_descriptor_heap)
    ));

    return srv_descriptor_heap;
}

ComPtr<ID3D12Resource> IApplication::_pimpl_create_dsv(
    ComPtr<ID3D12Device2> device,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_descriptor,
    const uint16_t window_width,
    const uint16_t window_height)
{
    D3D12_CLEAR_VALUE optimized_clear_value = {};
    optimized_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
    optimized_clear_value.DepthStencil = { 1.f, 0 };

    Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer;
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            window_width,
            window_height,
            1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimized_clear_value,
        IID_PPV_ARGS(&depth_buffer)
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.Texture2D.MipSlice = 0;
    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    device->CreateDepthStencilView(
        depth_buffer.Get(),
        &dsv_desc,
        dsv_descriptor
    );

    return depth_buffer;
}

void IApplication::_pimpl_create_backbuffers(
    ComPtr<ID3D12Device2> device,
    ComPtr<IDXGISwapChain4> swap_chain,
    ID3D12DescriptorHeap* descriptor_heap,
    ComPtr<ID3D12Resource>* backbuffers,
    uint8_t n_backbuffers)
{
    UINT rtv_desc_size = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV
    );
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
        descriptor_heap->GetCPUDescriptorHandleForHeapStart()
    );

    for (uint8_t i = 0; i < n_backbuffers; ++i)
    {
        ComPtr<ID3D12Resource> backbuffer;
        swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffer));

        device->CreateRenderTargetView(
            backbuffer.Get(),
            nullptr,
            rtv_handle
        );

        backbuffers[i] = backbuffer;

        rtv_handle.Offset(rtv_desc_size);
    }
}

ComPtr<ID3D12CommandAllocator> IApplication::_pimpl_create_command_allocator(
    ComPtr<ID3D12Device2> device,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> command_allocator;
    ThrowIfFailed(device->CreateCommandAllocator(
        type,
        IID_PPV_ARGS(&command_allocator)
    ));

    return command_allocator;
}

ComPtr<ID3D12CommandList> IApplication::_pimpl_create_command_list_copy(
    ComPtr<ID3D12Device2> device,
    ComPtr<ID3D12CommandAllocator> command_allocator,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandList> command_list_copy;
    ThrowIfFailed(device->CreateCommandList(
        0,
        type,
        command_allocator.Get(),
        NULL,
        IID_PPV_ARGS(&command_list_copy)
    ));

    return command_list_copy;
}

ComPtr<ID3D12GraphicsCommandList> IApplication::_pimpl_create_command_list(
    ComPtr<ID3D12Device2> device,
    ComPtr<ID3D12CommandAllocator> command_allocator,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> command_list_direct;
    ThrowIfFailed(device->CreateCommandList(
        0,
        type,
        command_allocator.Get(),
        NULL,
        IID_PPV_ARGS(&command_list_direct)
    ));

    return command_list_direct;
}

ComPtr<ID3D12Fence> IApplication::_pimpl_create_fence(
    ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(
        0,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence)
    ));

    return fence;
}

HANDLE IApplication::_pimpl_create_fence_event()
{
    return ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

}
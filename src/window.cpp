#include "window.hpp"

#include <cassert>
#include <functional>
#include <utility>

using namespace Microsoft::WRL;

namespace moonlight {

Window::Window( HINSTANCE hinstance,
                const wchar_t* window_class_name,
                const wchar_t* window_title,
                uint32_t width,
                uint32_t height,
                WNDPROC wndproc,
                void* parent_pointer) 
{
#ifdef _DEBUG
    enable_debug_layer();
#endif
    register_window_class(hinstance, window_class_name, wndproc);
    create_window(hinstance, window_class_name, window_title, width, height, parent_pointer);

    fullscreen = false;
    vsync = true;
}

//
//
// Implementation details
void Window::enable_debug_layer() {
    ComPtr<ID3D12Debug> debug_interface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
    debug_interface->EnableDebugLayer();
}

BOOL Window::check_tearing_support() {
    BOOL allow_tearing = FALSE;
    ComPtr<IDXGIFactory4> factory4;

    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) 
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5))) 
        {
            if (FAILED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allow_tearing,
                sizeof(allow_tearing))))
            {
                // Is this necessary? Wouldn't allow_tearing stay false,
                // if the function fails?
                allow_tearing = FALSE;
            }
        }
    }

    return allow_tearing;
}

bool Window::tearing_supported() {
    return check_tearing_support();
}

void Window::register_window_class(HINSTANCE hinstance, const wchar_t* class_name, WNDPROC wndproc) {
    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.hInstance = hinstance;
    window_class.lpszClassName = class_name;
    // I don't understand what BRUSHES are. This is what I saw from a tutorial.
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    // Why is the first parameter NULL?
    window_class.hCursor = NULL;// LoadCursor(NULL, IDC_ARROW);
    // Why is the first parameter HINSTANCE?
    //window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    //window_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hIcon = LoadIcon(hinstance, IDI_APPLICATION);
    window_class.hIconSm = LoadIcon(hinstance, IDI_APPLICATION);
    window_class.lpfnWndProc = wndproc;
    window_class.lpszMenuName = NULL;
    window_class.style = CS_HREDRAW | CS_VREDRAW;

    // Note to reader: The ATOM type has nothing to do with atomic operations
    // Read: https://learn.microsoft.com/en-us/windows/win32/dataxchg/about-atom-tables
    //ATOM copy_atom = ::RegisterClassExW(&window_class);
    static ATOM atom = ::RegisterClassExW(&window_class);

    assert(atom > 0);
}

void Window::create_window(
    HINSTANCE hinstance,
    const wchar_t* window_class_name,
    const wchar_t* window_title,
    uint32_t width,
    uint32_t height,
    void* parent_pointer)
{
    const int screen_width = ::GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = ::GetSystemMetrics(SM_CYSCREEN);

    RECT window_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

    window_width = window_rect.right - window_rect.left;
    window_height = window_rect.bottom - window_rect.top;

    // Center the window, and clamp to the top-left corner if necessary.
    const int window_x = std::max<int>(0, (screen_width - window_width) / 2);
    const int window_y = std::max<int>(0, (screen_height - window_height) / 2);

    handle = CreateWindowExW(
        NULL,
        window_class_name,
        window_title,
        WS_OVERLAPPEDWINDOW,
        window_x,
        window_y,
        window_width,
        window_height,
        NULL,
        NULL,
        hinstance,
        parent_pointer
    );

    window_width = width;
    window_height = height;

    assert(handle && "Failed to create window");
}

//
//
// Public interface
bool Window::resize() 
{
    RECT client_rect = {};
    ::GetClientRect(handle, &client_rect);

    uint32_t new_width = client_rect.right - client_rect.left;
    uint32_t new_height = client_rect.bottom - client_rect.top;

    window_width = window_rect.right - window_rect.left;
    window_height = window_rect.bottom - window_rect.top;

    if (new_width != window_width || new_height != window_height){
        window_width = std::max(1u, window_width);
        window_height = std::max(1u, window_height);
        return true;
    }

    return false;
}

void Window::flip_fullscreen() 
{
    fullscreen = !fullscreen;
        
    if (fullscreen) {
        ::GetWindowRect(handle, &window_rect);
        window_width = window_rect.right - window_rect.left;
        window_height = window_rect.bottom - window_rect.top;

        UINT window_style = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

        ::SetWindowLongW(handle, GWL_STYLE, window_style);

        HMONITOR hmonitor = ::MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitor_info = {};
        monitor_info.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hmonitor, &monitor_info);

        ::SetWindowPos(
            handle,
            HWND_TOP,
            monitor_info.rcMonitor.left,
            monitor_info.rcMonitor.top,
            monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
            monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(handle, SW_MAXIMIZE);
    } else {
        ::SetWindowLong(handle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(
            handle,
            HWND_NOTOPMOST,
            window_rect.left,
            window_rect.top,
            window_rect.right - window_rect.left,
            window_rect.bottom - window_rect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(handle, SW_NORMAL);
    }
}

void Window::flip_vsync() {
    vsync = !vsync;
}

uint32_t Window::height() {
    return window_height;
}

uint32_t Window::width() {
    return window_width;
}

}
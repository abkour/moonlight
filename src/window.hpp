#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "helpers.h"

#include <algorithm> // for std::max()

namespace moonlight {

struct Window {

    Window( HINSTANCE hinstance,
            const wchar_t* window_class_name,
            const wchar_t* window_title,
            uint32_t width, 
            uint32_t height,
            WNDPROC wndproc,
            void* parent_pointer);

    bool is_fullscreen_on() const { return m_fullscreen; }
    void flip_fullscreen();

    bool is_vsync_on() const { return m_vsync; }
    void flip_vsync();

    bool resize();

    HWND handle;

    bool tearing_supported();

    uint32_t height();
    uint32_t width();

    float aspect_ratio() const
    {
        return static_cast<float>(m_window_width) / static_cast<float>(m_window_height);
    }

private:

    // Implementation details
    
    void enable_debug_layer();
    BOOL check_tearing_support();
    void register_window_class(HINSTANCE hinstance, const wchar_t* class_name, WNDPROC wndproc);
    void create_window( HINSTANCE hinstance,
                        const wchar_t* window_class_name,
                        const wchar_t* window_title,
                        uint32_t width,
                        uint32_t height,
                        void* parent_pointer);

private:

    uint32_t m_window_width;
    uint32_t m_window_height;
    RECT m_window_rect;

    bool m_fullscreen;
    bool m_vsync;
};

}
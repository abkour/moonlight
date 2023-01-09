// moonlight.cpp : Defines the entry point for the application.

#include "framework.h"

#include "application.hpp"
#include "demos/cube_application/cube_application.hpp"
#include "demos/basic_sphere/basic_sphere.hpp"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    try {
        moonlight::BasicSphereApplication application(hInstance);
        application.run();
    } 
    catch (...) {
        OutputDebugStringA("[Moonlight] Unexpected exception somewhere!");
    }

    return 0;
}

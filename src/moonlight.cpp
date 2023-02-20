// moonlight.cpp : Defines the entry point for the application.

#include "framework.h"

#include "application.hpp"
#include "demos/00_cube_application/cube_application.hpp"
#include "demos/01_frustum_culling/frustum_culling.hpp"
#include "demos/02_environment_mapping/environment_mapping.hpp"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    try {
        moonlight::EnvironmentMapping application(hInstance);
        application.run();
    } 
    catch (...) {
        OutputDebugStringA("[Moonlight] Unexpected exception somewhere!");
    }
    
    return 0;
}

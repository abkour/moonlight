// moonlight.cpp : Defines the entry point for the application.

#include "framework.h"

#include "application.hpp"
#include "demos/00_cube_application/cube_application.hpp"
#include "demos/01_frustum_culling/frustum_culling.hpp"
#include "demos/02_environment_mapping/environment_mapping.hpp"
#include "demos/03_global_illumination/rtx_renderer.hpp"
#include "demos/04_plotter/plotter.hpp"
#include "demos/05_pbr/pbr_demo.hpp"
#include "demos/06_tetris/tetris.hpp"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    try 
    {
        moonlight::Tetris application(hInstance);
        application.run();
    } 
    catch (...) 
    {
        OutputDebugStringA("[Moonlight] Unexpected exception somewhere!");
    }
    
    return 0;
}

#include "profiler.hpp"
#include "utility/random_number.hpp"

#include "../imgui/imgui_internal.h"
#include "../imgui/implot.h"
#include "../imgui/implot_internal.h"

namespace moonlight
{

//-----------------------------------------------------------------------------

Profiler::Profiler(
    unsigned pGraphResolution,
    unsigned pUpdatesPerSecond,
    unsigned pInitialNumberOfGraphs,
    float pScaleMin, float pScaleMax)
{
    ImPlot::CreateContext();
}

//-----------------------------------------------------------------------------

void Profiler::add_point(float plot_point, int graph_idx)
{
    if (graph_idx >= 0 && graph_idx < m_graphs.size())
    {
        m_graphs[graph_idx].AddPoint(m_time, plot_point);
    }
}

//-----------------------------------------------------------------------------

void Profiler::update_internal_timers(float frame_time)
{
    m_frame_time = frame_time;
    m_time_since_window_name_update += frame_time;
    
    m_time += frame_time;
}

//-----------------------------------------------------------------------------

void Profiler::display_vram_info()
{
    char vram_info[128];
    sprintf_s(vram_info, "\nVirtual Memory (KB): %d/%d", m_vram_used_kb, m_vram_capacity_kb);;
    ImGui::Text(vram_info);
}

//-----------------------------------------------------------------------------

int Profiler::register_graph(const char* graph_name)
{
    m_graphs.push_back(graph_name);

    return m_graphs.size() - 1;
}

//-----------------------------------------------------------------------------

void Profiler::Demo_RealtimePlots() 
{
    static float history = 10.0f;
    ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 150))) 
    {
        ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, m_time - history, m_time, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        for (auto& graph : m_graphs)
        {
            ImPlot::PlotLine(graph.BufferName, &graph.Data[0].x, &graph.Data[0].y, graph.Data.size(), 0, graph.Offset, 2 * sizeof(float));
        }
        ImPlot::EndPlot();
    }
}

//-----------------------------------------------------------------------------

void Profiler::display(ID3D12GraphicsCommandList* command_list)
{
    char window_name[128];
    unsigned FPS = 1.f / m_frame_time;
    sprintf_s(
        window_name,
        "Profiler [FPS: %d\tFrame time: %.3fms] ###ProfilerWindow",
        FPS, m_frame_time
    );

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin(window_name);
    Demo_RealtimePlots();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(
        ImGui::GetDrawData(),
        command_list
    );
}

}
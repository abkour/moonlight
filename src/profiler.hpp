#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include <vector>

namespace moonlight
{

// The owner of this object is responsible for initializing and destroying
// the ImGui context. This object does not mutate ImGui objects.
class Profiler
{
    // Helper class for the plotting of the graphs
    struct ScrollingBuffer {
        const char* BufferName;
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;
        
        ScrollingBuffer(const char* buffer_name, int max_size = 2000) 
        {
            BufferName = buffer_name;
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }

        ScrollingBuffer(const ScrollingBuffer& other)
            : BufferName(other.BufferName)
            , MaxSize(other.MaxSize)
            , Offset(other.Offset)
        {
            Data.resize(MaxSize);
            std::memcpy(Data.Data, other.Data.Data, sizeof(ImVec2) * other.Data.size());
        }

        ScrollingBuffer& operator=(const ScrollingBuffer& other)
        {
            BufferName = other.BufferName;
            MaxSize = other.MaxSize;
            Offset = other.Offset;

            Data.resize(MaxSize);
            std::memcpy(Data.Data, other.Data.Data, sizeof(ImVec2) * other.Data.size());
        }

        void AddPoint(float x, float y) {
            if (Data.size() < MaxSize)
                Data.push_back(ImVec2(x, y));
            else {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }
        
        void Erase() {
            if (Data.size() > 0) {
                Data.shrink(0);
                Offset = 0;
            }
        }
    };

public:

    // pGraphResolution: The number of graph points visible in the plot.
    Profiler(
        unsigned pGraphResolution, 
        unsigned pUpdatesPerSecond = 25, 
        unsigned pInitialNumberOfGraphs = 1,
        float scale_min = 0.f,
        float scale_max = 0.33f
    );

    void add_point(float plot_point, int graph_idx);

    void display(ID3D12GraphicsCommandList* command_list);

    // Internally registers graph with name 'graph_name' and returns an identifier 
    // for that graph for use in the add_point function.
    int register_graph(const char* graph_name);

    void update_internal_timers(float frame_time);

    void set_vram_capacity(uint64_t vram_capacity_in_kb)
    {
        m_vram_capacity_kb = vram_capacity_in_kb * 0.001;
    }

    void set_vram_used(unsigned vram_used_in_kb)
    {
        m_vram_used_kb = vram_used_in_kb * 0.001;
    }

private:

    void display_vram_info();

    void Demo_RealtimePlots();

private:

    std::vector<ScrollingBuffer> m_graphs;

    float m_time = 0.f;
    float m_frame_time = 0.f;
    float m_window_update_interval = 0.1f;
    float m_time_since_window_name_update = 0.f;

    unsigned m_vram_capacity_kb;
    unsigned m_vram_used_kb;
};

}
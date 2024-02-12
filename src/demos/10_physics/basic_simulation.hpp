#pragma once
#include "../../application.hpp"
#include "../../core/pso.hpp"
#include "../../core/render_texture.hpp"
#include "../../core/vertex_buffer_resource.hpp"
#include "../../profiler.hpp"

namespace moonlight
{

class BasicSimulation : public IApplication
{

public:

    BasicSimulation(HINSTANCE);
    ~BasicSimulation();

    void resize() override;
    void render() override;
    void update() override;

private:

    void apply_gravity();

    void initialize_simulation();
    void initialize_d3d12_objects();

    void record_command_list();
    void record_gui_commands();

    void update_gui_state();

private:

    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;

    VertexBufferResource m_vb_circles;
    VertexBufferResource m_vb_triangle;
    std::unique_ptr<PipelineStateObject> m_pso_wrapper;
    std::unique_ptr<PipelineStateObject> m_pso_wrapper_ball;
    std::unique_ptr<DX12Resource> m_srv_buffer;
    std::unique_ptr<DX12Resource> m_srv_colors;

private:

    void apply_constraints(int ball_idx, float dt);
    void verlet_integration(int idx);

    struct BallPosition
    {
        // The constructor of the union is implicitly deleted, because Vector2 has a non-trivial
        // default constructor. Therefore, we are forced to define a constructor ourselves if 
        // we want to use make_unique. Sample applies to class BallVelocity.
        BallPosition() {}

        union
        {
            struct
            {
                float x, y;
            };
            Vector2<float> position;
        };
    };

    struct BallVelocity
    {
        BallVelocity() {}

        union
        {
            struct
            {
                float x, y;
            };
            Vector2<float> velocity;
        };
    };

    std::unique_ptr<BallPosition[]> m_ball_positions;
    std::unique_ptr<BallVelocity[]> m_ball_velocities;
    std::unique_ptr<Vector3<float>[]> m_ball_colors;

    int m_num_balls;
    int m_ball_capacity;
    float m_ball_radius;
    Vector2<float> m_simulation_scale;
    Vector2<float> m_triangle_offset;
    Vector2<float> m_emitter_position;
    Vector2<float> m_emitter_direction;
    float m_emitter_angle;

    float m_frame_time;
    float m_total_time;
    float m_time_since_plot_update;

    Profiler m_profiler;
    int m_plot0, m_plot1, m_plot2;

    D3D12_CPU_DESCRIPTOR_HANDLE m_test_handle;
};

}
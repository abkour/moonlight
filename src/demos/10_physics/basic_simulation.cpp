#include "basic_simulation.hpp"
#include <chrono>
#include "../../utility/random_number.hpp"

namespace moonlight
{

const float triangle[] =
{
    0.f, -0.1f, 0.25f, 0.f, 0.f, 0.1f
};

static void generate_circle_vertices(std::vector<float>& vertices, const int num_vertices, const float scale)
{
    const float angle_inc = radians(360.f) / static_cast<float>(num_vertices);

    for (int i = 0; i < num_vertices; ++i)
    {
        vertices.push_back(0.f);
        vertices.push_back(0.f);
        vertices.push_back(scale * std::cos(angle_inc * i));
        vertices.push_back(scale * std::sin(angle_inc * i));
        vertices.push_back(scale * std::cos(angle_inc * (i + 1)));
        vertices.push_back(scale * std::sin(angle_inc * (i + 1)));
    }
}

//-----------------------------------------------------------------------------

BasicSimulation::BasicSimulation(HINSTANCE hinstance)
    : IApplication(hinstance, &BasicSimulation::WindowMessagingProcess, Vector2<UINT>(900, 900))
    , m_profiler(36)
{
    m_plot0 = m_profiler.register_graph("frame_time");

    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3
    );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(m_window->handle);
    ImGui_ImplDX12_Init(
        m_device.Get(),
        3,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        m_srv_descriptor_heap->get_underlying(),
        m_srv_descriptor_heap->cpu_handle(2),
        m_srv_descriptor_heap->gpu_handle(2)
    );

    initialize_simulation();

    m_application_initialized = true;
}

//-----------------------------------------------------------------------------

BasicSimulation::~BasicSimulation()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//-----------------------------------------------------------------------------

void BasicSimulation::resize()
{
}

//-----------------------------------------------------------------------------

void BasicSimulation::apply_constraints(int ball_idx, float dt)
{
    auto& pa = m_ball_positions[ball_idx];
    auto& va = m_ball_velocities[ball_idx];
    for (int i = ball_idx + 1; i < m_num_balls; ++i)
    {
        auto& pb = m_ball_positions[i];
        auto& vb = m_ball_velocities[i];
        Vector2<float> collision_axis = pb.position - pa.position;
        float l = length(collision_axis);
        
        if (l < 2 * m_ball_radius)
        {
            Vector2<float> va = m_ball_velocities[ball_idx].velocity;
            Vector2<float> vb = m_ball_velocities[i].velocity;

            Vector2<float> delta_v = va - vb;
            Vector2<float> delta_x = pa.position - pb.position;
            float length_x_sq = length(delta_x);
            length_x_sq *= length_x_sq;

            Vector2<float> new_va = va - (dot(delta_v, delta_x) / (length_x_sq)) * delta_x;
            Vector2<float> new_vb = va + vb - new_va;

            m_ball_velocities[ball_idx].velocity = new_va;
            m_ball_velocities[i].velocity = new_vb;

            m_ball_positions[ball_idx].position += m_ball_velocities[ball_idx].velocity * dt;
            m_ball_positions[i].position += m_ball_velocities[i].velocity * dt;
        }
    }
}

//-----------------------------------------------------------------------------

void BasicSimulation::verlet_integration(int idx)
{
    float g = -9.81f; g = 0.f;
    float t = m_frame_time;

    m_ball_velocities[idx].velocity.y = m_ball_velocities[idx].velocity.y + g * t;

    m_ball_positions[idx].position 
        = m_ball_positions[idx].position + m_ball_velocities[idx].velocity * t + 0.5 * g * t * t;
}

//-----------------------------------------------------------------------------

void BasicSimulation::update()
{
    update_gui_state();

    // Collision walls
    float x0 = 0.f;
    float y0 = 0.f;
    float x1 = m_simulation_scale.x;
    float y1 = m_simulation_scale.y;

    static float threshold_time = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_frame_time = (t1 - t0).count() * 1e-9;
    m_total_time += m_frame_time;
    m_time_since_plot_update += m_frame_time;
    t0 = t1;

    static int test = true;
    float fire_interval = 0.02f;
    if (m_total_time >= fire_interval && test)
    {
        float r = 1.f;

        m_ball_positions[0].x = m_simulation_scale.x / 2.f - 1.f;
        m_ball_positions[0].y = m_simulation_scale.y / 2.f;
        m_ball_velocities[0].x = r * std::cos(m_emitter_angle);
        m_ball_velocities[0].y = 0 * std::sin(m_emitter_angle);
        m_ball_colors[0] = Vector3<float>(
            1.f, 0.f, 0.f
        );

        m_ball_positions[1].x = m_simulation_scale.x / 2.f + 1.f;
        m_ball_positions[1].y = m_simulation_scale.y / 2.f;
        m_ball_velocities[1].x = -r * std::cos(m_emitter_angle);
        m_ball_velocities[1].y = 0 * std::sin(m_emitter_angle);
        m_ball_colors[1] = Vector3<float>(
            0.f, 1.f, 0.f
        );
        
        test = false;

        m_num_balls = 2;

        /*
        if (m_num_balls + 1 < m_ball_capacity)
        {
            m_ball_positions[m_num_balls].x = m_simulation_scale.x / 2.f;
            m_ball_positions[m_num_balls].y = m_simulation_scale.y / 2.f;
            m_ball_velocities[m_num_balls].x = 100.f * std::cos(m_emitter_angle);
            m_ball_velocities[m_num_balls].y = 100.f * std::sin(m_emitter_angle);
            m_ball_colors[m_num_balls] = Vector3<float>(
                random_in_range(0.f, 1.f), 
                random_in_range(0.f, 1.f), 
                random_in_range(0.f, 1.f)
            );
            m_num_balls++;
            m_total_time = 0.f;
        }*/
    }

    static bool K_Pressed = false;
    if (m_keyboard_state['K'] == PackedKeyArguments::KeyState::Pressed && !K_Pressed)
    {
        K_Pressed = true;
    }
    if (m_keyboard_state['K'] == PackedKeyArguments::KeyState::Released && K_Pressed)
    {
        K_Pressed = false;
        
        // Verify disjoint circles condition
        for (int i = 0; i < m_num_balls; ++i)
        {
            auto p0 = m_ball_positions[i].position;
            for (int j = i + 1; j < m_num_balls; ++j)
            {
                auto p1 = m_ball_positions[j].position;
                auto diff = p1 - p0;
                float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                if (dist < m_ball_radius * 2.f)
                {
                    OutputDebugStringA("Disjoint circles condition violated!\n");
                    goto outside_loop;
                }
            }
        }

    outside_loop:
        return;
    }

    static bool T_Pressed = false;
    if (m_keyboard_state['T'] == PackedKeyArguments::KeyState::Pressed && !T_Pressed)
    {
        T_Pressed = true;
    }
    if (m_keyboard_state['T'] == PackedKeyArguments::KeyState::Released && T_Pressed)
    {
        T_Pressed = false;
        //m_swap_chain->take_screenshot(nullptr);
        uint8_t bb_index = m_swap_chain->current_backbuffer_index();
        auto cpu_handle = m_swap_chain->backbuffer_rtv_descriptor_handle(bb_index);
        

    }

    for (int i = 0; i < m_num_balls; ++i)
    {
        verlet_integration(i);
        apply_constraints(i, m_frame_time);

        if (m_ball_positions[i].y - m_ball_radius <= y0)
        {
            m_ball_velocities[i].x /= 2.f;
            m_ball_velocities[i].y /= -2.f;
            m_ball_positions[i].y = y0 + m_ball_radius;
        }
        
        if (m_ball_positions[i].y + m_ball_radius >= y1)
        {
            m_ball_velocities[i].x /= 2.f;
            m_ball_velocities[i].y /= -2.f;
            m_ball_positions[i].y = y1 - m_ball_radius;
        }
        
        if (m_ball_positions[i].x - m_ball_radius <= x0)
        {
            m_ball_velocities[i].x /= -2.f;
            m_ball_velocities[i].y /= 2.f;
            m_ball_positions[i].x = x0 + m_ball_radius;
        }

        if (m_ball_positions[i].x + m_ball_radius >= x1)
        {
            m_ball_velocities[i].x /= -2.f;
            m_ball_velocities[i].y /= 2.f;
            m_ball_positions[i].x = x1 - m_ball_radius;
        }
    }
    
    m_srv_buffer->update(
        m_device.Get(), 
        m_command_list_direct.Get(), 
        m_ball_positions.get(),
        sizeof(Vector2<float>) * m_num_balls,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    m_srv_colors->update(
        m_device.Get(),
        m_command_list_direct.Get(),
        m_ball_colors.get(),
        sizeof(Vector3<float>) * m_num_balls,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
}

//-----------------------------------------------------------------------------

void BasicSimulation::update_gui_state()
{
    if (m_mouse.last_event() != MouseInterface::MouseEvents::NONE)
    {
        ImGuiIO& io = ImGui::GetIO();

        switch (m_mouse.last_event())
        {
        case MouseInterface::MouseEvents::LMB_Pressed:
            io.MouseDown[0] = true;
            break;
        case MouseInterface::MouseEvents::LMB_Released:
            io.MouseDown[0] = false;
            break;
        case MouseInterface::MouseEvents::Wheel_Scroll:
            io.MouseWheel = (float)m_mouse.wheel_delta() / WHEEL_DELTA;
            break;
        default:
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void BasicSimulation::render()
{
    IApplication::clear_rtv_dsv(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.f));

    record_command_list();
    record_gui_commands();

    IApplication::present();
}

//-----------------------------------------------------------------------------

void BasicSimulation::record_command_list()
{
    const Vector2<float> inv_scale = inverse(m_simulation_scale);
    const Vector2<float> tri_scale(0.2f, 0.2f);

    m_emitter_angle += 0.01f;

    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vb_circles.get_view(), m_vb_triangle.get_view()};

    m_command_list_direct->SetPipelineState(m_pso_wrapper_ball->pso());
    m_command_list_direct->SetGraphicsRootSignature(m_pso_wrapper_ball->root_signature());
    m_command_list_direct->IASetVertexBuffers(0, 1, vb_views);
    m_command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_command_list_direct->SetGraphicsRoot32BitConstants(0, 2, &inv_scale, 0);
    m_command_list_direct->SetGraphicsRootShaderResourceView(
        1, m_srv_buffer->gpu_virtual_address()
    );
    m_command_list_direct->SetGraphicsRootShaderResourceView(
        2, m_srv_colors->gpu_virtual_address()
    );
    m_command_list_direct->RSSetViewports(1, &m_viewport);
    m_command_list_direct->RSSetScissorRects(1, &m_scissor_rect);
    m_command_list_direct->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
    m_command_list_direct->DrawInstanced(108, m_num_balls, 0, 0);

    m_command_list_direct->SetPipelineState(m_pso_wrapper->pso());
    m_command_list_direct->SetGraphicsRootSignature(m_pso_wrapper->root_signature());
    m_command_list_direct->SetGraphicsRoot32BitConstants(0, 2, &tri_scale, 0);
    m_command_list_direct->SetGraphicsRoot32BitConstants(0, 1, &m_emitter_angle, 2);
    m_command_list_direct->IASetVertexBuffers(0, 1, &vb_views[1]);
    m_command_list_direct->DrawInstanced(3, 1, 0, 0);
}

//-----------------------------------------------------------------------------

void BasicSimulation::record_gui_commands()
{
    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };
    m_command_list_direct->SetDescriptorHeaps(1, heaps);

    m_profiler.update_internal_timers(m_frame_time);

    if (m_time_since_plot_update > 0.04f)
    {
        m_profiler.add_point(m_frame_time, m_plot0);
        m_time_since_plot_update = 0.f;
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO vram_info;
    m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vram_info);
    m_profiler.set_vram_capacity(vram_info.Budget);
    m_profiler.set_vram_used(vram_info.CurrentUsage);

    m_profiler.display(m_command_list_direct.Get());
}

//-----------------------------------------------------------------------------

void BasicSimulation::initialize_simulation()
{
    float aspect_ratio = (float)m_window->height() / (float)m_window->width();
    m_simulation_scale = Vector2<float>(100.f, 100.f);
    m_emitter_position = Vector2<float>(0.f, 0.f);
    m_ball_radius = 2.f;
    m_num_balls = 0;
    m_ball_capacity = 100;
    m_emitter_angle = 0.f;

    m_ball_positions = std::make_unique<BallPosition[]>(m_ball_capacity);
    m_ball_velocities = std::make_unique<BallVelocity[]>(m_ball_capacity);
    m_ball_colors = std::make_unique<Vector3<float>[]>(m_ball_capacity);

    m_total_time = 0.f;
    m_time_since_plot_update = 0.f;

    initialize_d3d12_objects();
}

//-----------------------------------------------------------------------------

void BasicSimulation::initialize_d3d12_objects()
{
    std::vector<float> circle_vertices;
    generate_circle_vertices(
        circle_vertices, 36, m_ball_radius / (m_simulation_scale.x * 0.5f)
    );

    m_vb_circles.upload(
        m_device.Get(),
        m_command_list_direct.Get(),
        circle_vertices.size() / 2, 2, sizeof(float), (void*)circle_vertices.data()
    );

    m_vb_triangle.upload(
        m_device.Get(),
        m_command_list_direct.Get(),
        _countof(triangle) / 2, 2, sizeof(float), (void*)triangle
    );

    // Per Instance Ball Offset
    {
        m_srv_buffer = std::make_unique<DX12Resource>();
        m_srv_buffer->upload(
            m_device.Get(), m_command_list_direct.Get(),
            m_ball_positions.get(), sizeof(BallPosition) * m_ball_capacity,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_ball_capacity;
        buffer_desc.StructureByteStride = sizeof(BallPosition);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            m_srv_buffer->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle()
        );

        m_srv_buffer->transition(
            m_command_list_direct.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }

    // Per Instance Ball Color
    {
        m_srv_colors = std::make_unique<DX12Resource>();
        m_srv_colors->upload(
            m_device.Get(), m_command_list_direct.Get(),
            m_ball_colors.get(), sizeof(Vector3<float>) * m_ball_capacity,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_ball_capacity;
        buffer_desc.StructureByteStride = sizeof(Vector3<float>);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            m_srv_colors->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle()
        );

        m_srv_colors->transition(
            m_command_list_direct.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }

    std::wstring vspath
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/10_physics/shaders/triangle_vs.hlsl";
    std::wstring pspath
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/10_physics/shaders/triangle_ps.hlsl";

    std::wstring vspath_ball
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/10_physics/shaders/ball_vs.hlsl";
    std::wstring pspath_ball
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/10_physics/shaders/ball_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {   "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

    CD3DX12_ROOT_PARAMETER1 root_parameters_triangle[1];
    root_parameters_triangle[0].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    m_pso_wrapper = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_pso_wrapper->construct_input_layout(input_layout, _countof(input_layout));
    m_pso_wrapper->construct_root_signature(root_parameters_triangle, _countof(root_parameters_triangle), nullptr, 0);
    m_pso_wrapper->construct_rt_formats(rtv_formats);
    m_pso_wrapper->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_pso_wrapper->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_pso_wrapper->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_pso_wrapper->construct();

    CD3DX12_ROOT_PARAMETER1 root_parameters_ball[3];
    root_parameters_ball[0].InitAsConstants(2, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters_ball[1].InitAsShaderResourceView(0);
    root_parameters_ball[2].InitAsShaderResourceView(1);

    m_pso_wrapper_ball = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_pso_wrapper_ball->construct_input_layout(input_layout, _countof(input_layout));
    m_pso_wrapper_ball->construct_root_signature(root_parameters_ball, _countof(root_parameters_ball), nullptr, 0);
    m_pso_wrapper_ball->construct_rt_formats(rtv_formats);
    m_pso_wrapper_ball->construct_vs(vspath_ball.c_str(), L"main", L"vs_6_1");
    m_pso_wrapper_ball->construct_ps(pspath_ball.c_str(), L"main", L"ps_6_1");
    m_pso_wrapper_ball->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_pso_wrapper_ball->construct();
}

}
#include "tetris_app.hpp"
#include "input_buffer.hpp"
#include "../../simple_math.hpp"
#include "../../utility/random_number.hpp"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{

static const float quad_vertices[] =
{
    -1.f, -1.f,    1.f, -1.f,    1.f, 1.f,
    -1.f, -1.f,    1.f, 1.f,    -1.f, 1.f
};

// 10 width x 20 height

static const float borders[] =
{
    // Horizontal line at the bottom
    tetirs_border_xorigin, tetris_border_yorigin,
    tetirs_border_xorigin + tetris_width, tetris_border_yorigin,

    // Vertical line at the left
    tetirs_border_xorigin, tetris_border_yorigin,
    tetirs_border_xorigin, tetris_border_yorigin + tetris_height,

    // Vertical line at the right
    tetirs_border_xorigin + tetris_width, tetris_border_yorigin,
    tetirs_border_xorigin + tetris_width, tetris_border_yorigin + tetris_height
};

}

namespace moonlight
{

Tetris::Tetris(HINSTANCE hinstance)
    : IApplication(hinstance, &Tetris::WindowMessagingProcess)
    , m_input_buffer(12)
{
    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1
    );
    
    load_assets();

    std::wstring font_filename = std::wstring(ROOT_DIRECTORY_WIDE) + L"/rsc/myfile.spriteFont";
    glyph_renderer = std::make_unique<GlyphRenderer>(
        m_device.Get(),
        m_command_queue->get_underlying(),
        m_viewport,
        font_filename.c_str()
    );
    font_pos.x = static_cast<float>(m_window->width()) * 0.45;
    font_pos.y = static_cast<float>(m_window->height()) * 0.2;

    m_application_initialized = true;
}

Tetris::~Tetris()
{
}

void Tetris::on_key_event(const PackedKeyArguments key_state)
{
    enum KeyPressed
    {
        None = 0, A, D, W, Spacebar
    };

    static KeyPressed key_pressed = KeyPressed::None;

    static float key_update_timer = 0.f;

    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    float elapsed_time = (t1 - t0).count() * 1e-9;
    t0 = t1;
    key_update_timer += elapsed_time;

    static float key_press_start = 0.f;
    static float key_pressed_time = 0.f;

    static bool a_pressed = false;
    static bool d_pressed = false;
    static bool w_pressed = false;
    static bool space_pressed = false;
    static bool r_pressed = false;

    const float double_press_threshold = 0.0001f;

    if (key_pressed != KeyPressed::A && m_keyboard_state['A'])
    {
        key_pressed = KeyPressed::A;
        key_press_start = key_update_timer;
        key_pressed_time = 0.f;
    }
    else if (key_pressed == KeyPressed::A && m_keyboard_state['A'])
    {
        key_pressed_time = key_update_timer - key_press_start;
    }
    else if (key_pressed == KeyPressed::A && !m_keyboard_state['A'])
    {
        key_pressed_time = 0.f;
    }

    if (key_pressed != KeyPressed::D && m_keyboard_state['D'])
    {
        key_pressed = KeyPressed::D;
        key_press_start = m_total_time;
        key_pressed_time = 0.f;
    }
    else if (key_pressed == KeyPressed::D && m_keyboard_state['D'])
    {
        key_pressed_time = m_total_time - key_press_start;
    }
    else if (key_pressed == KeyPressed::D && !m_keyboard_state['D'])
    {
        key_pressed_time = 0.f;
    }

    if (m_keyboard_state['A'] && !a_pressed ||
        m_keyboard_state['A'])
    {
        a_pressed = true;
        m_input_buffer.push('A');
    }
    if (!m_keyboard_state['A'] && a_pressed)
    {
        a_pressed = false;
    }

    if (m_keyboard_state['D'] && !d_pressed ||
        m_keyboard_state['D'] && key_pressed_time > double_press_threshold)
    {
        d_pressed = true;
        m_input_buffer.push('D');
    }
    if (!m_keyboard_state['D'] && d_pressed)
    {
        d_pressed = false;
    }

    if (m_keyboard_state['W'] && !w_pressed)
    {
        w_pressed = true;
        m_input_buffer.push('W');
    }
    if (!m_keyboard_state['W'] && w_pressed)
    {
        w_pressed = false;
    }

    if (m_keyboard_state[KeyCode::Spacebar] && !space_pressed)
    {
        space_pressed = true;
        m_input_buffer.push(m_keyboard_state[KeyCode::Spacebar]);
    }
    if (!m_keyboard_state[KeyCode::Spacebar] && space_pressed)
    {
        space_pressed = false;
    }

    if (m_keyboard_state['R'] && !r_pressed)
    {
        r_pressed = true;
        m_input_buffer.push('R');
    }
}

void Tetris::on_mouse_move() 
{
}

void Tetris::render() 
{
    IApplication::clear_rtv_dsv(XMFLOAT4(0.05f, 0.05f, 0.05f, 1.f));

    record_command_list(m_command_list_direct.Get());

    IApplication::present();
}

void Tetris::resize() 
{
}

void Tetris::update() 
{
    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());

    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 500.f;
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    static unsigned game_score = 0;
    static bool space_pressed = false;
    static bool w_pressed = false;
    static float simulation_scale = 1.f;
    static float ms_threshold_time = 0.f;
    static float update_timer = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_elapsed_time = (t1 - t0).count() * 1e-9;
    t0 = t1;
    m_total_time += m_elapsed_time;
    ms_threshold_time += m_elapsed_time;
    update_timer += m_elapsed_time;
    
    static bool p_pressed = false;
    static bool game_paused = false;

    if (m_keyboard_state['P'] && !p_pressed)
    {
        p_pressed = true;
        game_paused = !game_paused;
    }

    if (!m_keyboard_state['P'] && p_pressed)
    {
        p_pressed = false;
    }

    if (game_paused)
    {
        return;
    }

    if (m_field->is_game_valid)
    {
        static TetrisBlock tetris_block = create_block();
        static bool is_collide = false;

        TetrisBlock highlight_block = m_field->quick_drop_highlight2(tetris_block);
        m_field->display(tetris_block);
        int lines_cleared = m_field->clear_lines();
        update_instanced_buffer();
        m_field->clear(tetris_block);
        m_field->clear(highlight_block);

        while (m_input_buffer.size() != 0)
        {
            auto key = m_input_buffer.front();
            m_input_buffer.pop();
            
            if (key == 'A')
            {
                tetris_block.move_left(m_field->simulation_grid);
            }
            if (key == 'D')
            {
                tetris_block.move_right(m_field->simulation_grid);
            }
            if (key == 'W')
            {
                tetris_block = rotate_block(tetris_block, m_field->simulation_grid);
            }
            if (key == m_keyboard_state[KeyCode::Spacebar])
            {
                m_field->quick_drop2(highlight_block);

                m_field->synchronize_grids();
                tetris_block = create_block();
                update_timer = 0.f;
            }
            if (key == 'R')
            {
                tetris_block = create_block();
                update_timer = 0.f;
                break;
            }
        }
         
        if (ms_threshold_time > 0.01f)
        {
            if (m_keyboard_state['S'])
            {
                simulation_scale = 0.15f;
            }
            if (!m_keyboard_state['S'])
            {
                simulation_scale = 1.f;
            }

            ms_threshold_time = 0.f;
        }

        if (update_timer > 0.1f)
        {
            is_collide = m_field->collide(tetris_block);

            m_field->synchronize_grids();

            if (is_collide)
            {
                tetris_block = create_block();
                is_collide = false;

                update_timer = 0.f;
            }
        }

        if (update_timer > 0.33f * simulation_scale)
        {
            tetris_block.descend();
            update_timer = 0.f;
        }

        switch (lines_cleared)
        {
        case 1:
            game_score += 250;
        case 2:
            game_score += 600;
            break;
        case 3:
            game_score += 950;
            break;
        case 4:
            game_score += 1600;
            break;
        }

        text_output.resize(128);
        swprintf(
            text_output.data(),
            L"%d",
            game_score
        );
    }
    else
    {
        swprintf(
            text_output.data(),
            L"Game over"
        );
    }
}

void Tetris::update_instanced_buffer()
{
    for (int y = 0; y < m_field->grid.size(); ++y)
    {
        for (int x = 0; x < m_field->grid[y].size(); ++x)
        {
            int i = y * tetris_width + x;
            instances_buffer[i] = m_field->grid[y][x];
        }
    }

    instance_buffer_rsc->update(
        m_device.Get(), m_command_list_direct.Get(),
        instances_buffer.data(), sizeof(unsigned) * instances_buffer.size(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    D3D12_BUFFER_SRV buffer_desc = {};
    buffer_desc.FirstElement = 0;
    buffer_desc.NumElements = m_field->grid.size();
    buffer_desc.StructureByteStride = sizeof(unsigned);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer = buffer_desc;
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    m_device->CreateShaderResourceView(
        instance_buffer_rsc->get_underlying(),
        &srv_desc,
        m_srv_descriptor_heap->cpu_handle()
    );
}

void Tetris::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    const float aspect_ratio 
        = static_cast<float>(m_window->width()) / m_window->height();

    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    UINT rtv_inc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };

    command_list->SetPipelineState(m_scene_pso->pso());
    command_list->SetGraphicsRootSignature(m_scene_pso->root_signature());
    command_list->SetGraphicsRootShaderResourceView(1, instance_buffer_rsc->gpu_virtual_address());

    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, nullptr);
    command_list->SetGraphicsRoot32BitConstants(0, 1, &aspect_ratio, 0);
    command_list->DrawInstanced(
        sizeof(quad_vertices) / (sizeof(float) * 2), 
        tetris_width * tetris_height, 
        0, 0
    );

    glyph_renderer->render_text(
        command_list, 
        m_command_queue->get_underlying(), 
        text_output.c_str(), 
        font_pos
    );
}

void Tetris::load_assets()
{
    m_field = std::make_unique<Playfield>();
    instances_buffer.resize(tetris_width * tetris_height, 0xFF);
    load_scene_shader_assets();
}

void Tetris::load_scene_shader_assets()
{
    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(),
            (float*)quad_vertices,
            sizeof(quad_vertices),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = sizeof(quad_vertices);
        m_vertex_buffer_view.StrideInBytes = sizeof(float) * 2;
    }

    // Instance data SRV
    {
        instance_buffer_rsc = std::make_unique<DX12Resource>();
        instance_buffer_rsc->upload(
            m_device.Get(), m_command_list_direct.Get(),
            instances_buffer.data(), sizeof(unsigned) * instances_buffer.size(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_field->grid.size();
        buffer_desc.StructureByteStride = sizeof(unsigned);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            instance_buffer_rsc->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle()
        );
    }

    std::wstring vspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"/src/demos/06_tetris/shaders/tetris_vs.hlsl";
    std::wstring pspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"/src/demos/06_tetris/shaders/tetris_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[] = 
    {
        {   "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        } 
    };

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    root_parameters[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsShaderResourceView(0);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    
    D3D12_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    m_scene_pso = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_scene_pso->construct_input_layout(input_layout, _countof(input_layout));
    m_scene_pso->construct_root_signature(root_parameters, _countof(root_parameters), nullptr, 0);
    m_scene_pso->construct_rt_formats(rtv_formats);
    m_scene_pso->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_scene_pso->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_scene_pso->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_scene_pso->construct();
}

}
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

static struct ScenePipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL ds_desc;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blend_desc;
} scene_pipeline_state_stream;

Tetris::Tetris(HINSTANCE hinstance)
    : IApplication(hinstance)
    , m_input_buffer(12)
{
    m_window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_Template",
        1600,
        800,
        &Tetris::WindowMessagingProcess,
        this
    );

    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    m_device = _pimpl_create_device(most_sutiable_adapter);
    m_command_queue = std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_allocator = _pimpl_create_command_allocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_list_direct = _pimpl_create_command_list(m_device, m_command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_dsv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2
    );

    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1
    );

    m_swap_chain = std::make_unique<SwapChain>(
        m_device.Get(),
        m_command_queue->get_underlying(),
        m_window->width(),
        m_window->height(),
        m_window->handle
    );

    m_scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );

    m_depth_buffer = _pimpl_create_dsv(
        m_device,
        m_dsv_descriptor_heap->cpu_handle(),
        m_window->width(), m_window->height()
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

void Tetris::flush() 
{
    m_command_queue->flush();
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
}

void Tetris::on_mouse_move(LPARAM) 
{
}

void Tetris::render() 
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();

    // Clear
    {
        m_swap_chain->transition_to_rtv(m_command_list_direct.Get());

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.05f, 0.05f, 0.05f, 1.f };
        m_command_list_direct->ClearRenderTargetView(
            m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        m_command_list_direct->ClearDepthStencilView(
            m_dsv_descriptor_heap->cpu_handle(),
            D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL
        );
    }

    record_command_list(m_command_list_direct.Get());
    record_gui_commands(m_command_list_direct.Get());

    // Present
    {
        m_swap_chain->transition_to_present(m_command_list_direct.Get());

        m_command_list_direct->Close();
        ID3D12CommandList* command_lists[] =
        {
            m_command_list_direct.Get()
        };

        m_command_queue->execute_command_list(command_lists, 1);
        m_command_queue->signal();
        m_swap_chain->present();
        m_command_queue->wait_for_fence();

        ThrowIfFailed(m_command_allocator->Reset());
        ThrowIfFailed(m_command_list_direct->Reset(m_command_allocator.Get(), nullptr));
    }
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

    float bsx = 0.0125f;
    float bsy = 0.025f;
    m_mvp_matrix = XMMatrixScaling(bsx, bsy, 0.f);

    float pfx = 0.f;
    float pfy = 0.5f;
    XMMATRIX translate_matrix = XMMatrixTranslation(pfx, pfy, 0.f);
    m_mvp_matrix = XMMatrixMultiply(m_mvp_matrix, translate_matrix);

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

void Tetris::record_gui_commands(ID3D12GraphicsCommandList* command_list)
{
}

void Tetris::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_dsv_descriptor_heap->cpu_handle();
    UINT rtv_inc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    //
    // Render Scene to Texture
    command_list->SetPipelineState(m_scene_pso.Get());
    command_list->SetGraphicsRootSignature(m_scene_root_signature.Get());
    command_list->SetGraphicsRootShaderResourceView(1, instance_buffer_rsc->gpu_virtual_address());

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_mvp_matrix, 0);
    command_list->DrawInstanced(sizeof(quad_vertices) / (sizeof(float) * 2), tetris_width * tetris_height, 0, 0);

    glyph_renderer->render_text(command_list, m_command_queue->get_underlying(), text_output.c_str(), font_pos);
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

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/06_tetris/shaders/tetris_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/06_tetris/shaders/tetris_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] = 
    {
        {   "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        } 
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    // Three float4x4
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsShaderResourceView(0);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(
        _countof(root_parameters), root_parameters,
        0, nullptr,
        root_signature_flags
    );

    ComPtr<ID3DBlob> root_signature_blob;
    // TODO: What is the error blob=
    ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &root_signature_desc,
        feature_data.HighestVersion,
        &root_signature_blob,
        &error_blob
    ));

    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&m_scene_root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    
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

    scene_pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = m_scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    scene_pipeline_state_stream.ds_desc = dsv_desc;
    scene_pipeline_state_stream.blend_desc = CD3DX12_BLEND_DESC(blend_desc);

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(m_device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&m_scene_pso)));
}

}
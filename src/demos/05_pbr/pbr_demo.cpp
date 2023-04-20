#include "pbr_demo.hpp"
#include "../../simple_math.hpp"

using namespace Microsoft::WRL;
using namespace DirectX;



namespace
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
} scene_pipeline_state_stream;

struct VertexFormat
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
};

static float cube_vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

}

namespace moonlight
{

struct PSAttributes
{
    Vector3<float> position; float dummy0;
    Vector3<float> direction; float dummy1;
    Vector3<float> luminance; float dummy2;
    Vector3<float> view_position; float dummy3;
    Vector3<float> view_direction; float dummy4;
    Vector3<float> albedo; float dummy5;
    Vector3<float> metallic_roughness_ao; float dummy6;
};

alignas(256) static PSAttributes ps_00;

PBRDemo::PBRDemo(HINSTANCE hinstance)
    : IApplication(hinstance)
    , m_camera(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), 10.f)
{
    ps_00.position = Vector3<float>(5.f, 5.f, 0.f);
    ps_00.direction = normalize(invert(ps_00.position));
    ps_00.luminance = Vector3<float>(15.f, 15.f, 15.f);
    ps_00.albedo = Vector3<float>(0.5f, 0.f, 0.f);
    ps_00.metallic_roughness_ao.x = 1.f;
    ps_00.metallic_roughness_ao.y = 0.5f;
    ps_00.metallic_roughness_ao.z = 1.f;

    m_window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_Template",
        1600,
        800,
        &PBRDemo::WindowMessagingProcess,
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

    {
        // IMGUI initialization
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
            m_srv_descriptor_heap->cpu_handle(0),
            m_srv_descriptor_heap->gpu_handle(0)
        );
    }

    load_assets();

    m_application_initialized = true;
}

PBRDemo::~PBRDemo()
{}

void PBRDemo::flush()
{
    m_command_queue->flush();
}

void PBRDemo::on_key_event(const PackedKeyArguments key_state)
{
    ImGuiIO& io = ImGui::GetIO();

    if (key_state.key < 256)
    {
        io.KeysDown[key_state.key] = key_state.key_state;
    }
}

void PBRDemo::on_mouse_move(LPARAM lparam)
{
    UINT size;

    GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
    LPBYTE lpb = new BYTE[size];
    if (lpb == NULL) return;

    if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &size, sizeof(RAWINPUTHEADER)) != size)
    {
        OutputDebugStringA("GetRawInputData does not report correct size");
    }
    RAWINPUT* raw = (RAWINPUT*)lpb;
    if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        ImGuiIO& io = ImGui::GetIO();
        switch (raw->data.mouse.usButtonFlags)
        {
        case RI_MOUSE_BUTTON_1_DOWN:
            io.MouseDown[0] = true;
            break;
        case RI_MOUSE_BUTTON_1_UP:
            io.MouseDown[0] = false;
            break;
        case RI_MOUSE_WHEEL:
            io.MouseWheel = (float)(short)raw->data.mouse.usButtonData / WHEEL_DELTA;
            break;
        default:
            break;
        }
        if (m_keyboard_state[KeyCode::Shift])
        {
            m_camera.rotate(-raw->data.mouse.lLastX, -raw->data.mouse.lLastY);
        }
    }

    delete[] lpb;
}

void PBRDemo::render() 
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

void PBRDemo::resize() 
{
}

void PBRDemo::update() 
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

    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_elapsed_time = (t1 - t0).count() * 1e-9;
    t0 = t1;

    if (m_keyboard_state.keys[KeyCode::Shift])
    {
        m_camera.translate(m_keyboard_state, m_elapsed_time);
    }
    m_mvp_matrix = XMMatrixMultiply(m_camera.view, projection_matrix);

    XMFLOAT3 cpos = m_camera.get_position();
    XMFLOAT3 cdir = m_camera.get_direction();
    ps_00.view_position = Vector3<float>(cpos.x, cpos.y, cpos.z);
    ps_00.view_direction = Vector3<float>(cdir.x, cdir.y, cdir.z);

    float s = 0.25f;
    XMMATRIX model_matrix = XMMatrixScaling(s, s, s);
    XMMATRIX translate_matrix = XMMatrixTranslation(ps_00.position.x, ps_00.position.y, ps_00.position.z);
    model_matrix = XMMatrixMultiply(model_matrix, translate_matrix);
    m_cube_mvp = XMMatrixMultiply(model_matrix, m_camera.view);
    m_cube_mvp = XMMatrixMultiply(m_cube_mvp, projection_matrix);
}

void PBRDemo::record_gui_commands(ID3D12GraphicsCommandList* command_list)
{
    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };
    command_list->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        static int open_file_browser = 0;

        ImGui::Begin("Moonlight");
        
        ImGui::DragFloat("metallic", &ps_00.metallic_roughness_ao.x, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("roughness", &ps_00.metallic_roughness_ao.y, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("AO", &ps_00.metallic_roughness_ao.z, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("Albedo_x", &ps_00.albedo.x, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("Albedo_y", &ps_00.albedo.y, 0.01f, 0.f, 1.f);
        ImGui::DragFloat("Albedo_z", &ps_00.albedo.z, 0.01f, 0.f, 1.f);

        ImGui::Text("\nApplication average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_command_list_direct.Get());
}

void PBRDemo::record_command_list(ID3D12GraphicsCommandList* command_list)
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

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_mvp_matrix, 0);
    command_list->SetGraphicsRoot32BitConstants(1, sizeof(PSAttributes) / sizeof(float), &ps_00, 0);
    command_list->DrawInstanced(sizeof(cube_vertices) / sizeof(VertexFormat), 1, 0, 0);

    command_list->SetPipelineState(m_cube_pso.Get());
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_cube_mvp, 0);
    command_list->DrawInstanced(sizeof(cube_vertices) / sizeof(VertexFormat), 1, 0, 0);
}

void PBRDemo::load_assets()
{
    load_scene_shader_assets();
}

void PBRDemo::load_scene_shader_assets()
{
    // Vertex buffer uploading
    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(),
            (float*)cube_vertices,
            sizeof(cube_vertices),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = sizeof(cube_vertices);
        m_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/pbr_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/pbr_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_n(input_layout);

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
    root_parameters[1].InitAsConstants(sizeof(PSAttributes) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

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

    scene_pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = m_scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    scene_pipeline_state_stream.ds_desc = dsv_desc;

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(m_device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&m_scene_pso)));

    ComPtr<ID3DBlob> cube_vs_blob;
    ComPtr<ID3DBlob> cube_ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/cube_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/cube_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &cube_vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &cube_ps_blob));
    }

    ScenePipelineStateStream cube_pss;
    cube_pss.dsv_format = DXGI_FORMAT_D32_FLOAT;
    cube_pss.input_layout = { input_layout, _countof(input_layout) };
    cube_pss.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    cube_pss.ps = CD3DX12_SHADER_BYTECODE(cube_ps_blob.Get());
    cube_pss.root_signature = m_scene_root_signature.Get();
    cube_pss.rs = rasterizer_desc;
    cube_pss.rtv_formats = rtv_formats;
    cube_pss.vs = CD3DX12_SHADER_BYTECODE(cube_vs_blob.Get());
    cube_pss.ds_desc = dsv_desc;

    D3D12_PIPELINE_STATE_STREAM_DESC cube_pss_desc = {
        sizeof(ScenePipelineStateStream),
        &cube_pss
    };

    ThrowIfFailed(m_device->CreatePipelineState(&cube_pss_desc, IID_PPV_ARGS(&m_cube_pso)));
}

}
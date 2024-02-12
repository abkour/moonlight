#include "pbr_demo.hpp"
#include "../../simple_math.hpp"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace
{

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
    : IApplication(hinstance, &PBRDemo::WindowMessagingProcess)
{
    ps_00.position = Vector3<float>(5.f, 5.f, 0.f);
    ps_00.direction = normalize(invert(ps_00.position));
    ps_00.luminance = Vector3<float>(15.f, 15.f, 15.f);
    ps_00.albedo = Vector3<float>(0.5f, 0.f, 0.f);
    ps_00.metallic_roughness_ao.x = 1.f;
    ps_00.metallic_roughness_ao.y = 0.5f;
    ps_00.metallic_roughness_ao.z = 1.f;

    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1
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

void PBRDemo::on_mouse_move()
{
    ImGuiIO& io = ImGui::GetIO();

    switch (m_mouse.last_event())
    {
    case MouseInterface::LMB_Pressed:
        io.MouseDown[0] = true;
        break;
    case MouseInterface::LMB_Released:
        io.MouseDown[0] = false;
        break;
    default:
        break;
    }
    
    if (m_keyboard_state[KeyCode::Shift])
    {
        m_camera.rotate(-m_mouse.deltax(), -m_mouse.deltay());
    }

    /*
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
    */
}

void PBRDemo::render() 
{
    IApplication::clear_rtv_dsv(XMFLOAT4(0.05f, 0.05f, 0.05f, 1.f));

    record_command_list(m_command_list_direct.Get());
    record_gui_commands(m_command_list_direct.Get());

    IApplication::present();
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
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };

    //
    // Render Scene to Texture
    command_list->SetPipelineState(m_pso_wrapper->pso());
    command_list->SetGraphicsRootSignature(m_pso_wrapper->root_signature());

    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_mvp_matrix, 0);
    command_list->SetGraphicsRoot32BitConstants(1, sizeof(PSAttributes) / sizeof(float), &ps_00, 0);
    command_list->DrawInstanced(sizeof(cube_vertices) / sizeof(VertexFormat), 1, 0, 0);

    command_list->SetPipelineState(m_pso_wrapper->pso());
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

    std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/pbr_vs.hlsl";
    std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/pbr_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_n(input_layout);

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    // Three float4x4
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsConstants(sizeof(PSAttributes) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    m_pso_wrapper = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_pso_wrapper->construct_input_layout(input_layout, _countof(input_layout));
    m_pso_wrapper->construct_root_signature(root_parameters, _countof(root_parameters), nullptr, 0);
    m_pso_wrapper->construct_rt_formats(rtv_formats);
    m_pso_wrapper->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_pso_wrapper->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_pso_wrapper->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_pso_wrapper->construct_ds_format(DXGI_FORMAT_D32_FLOAT);
    m_pso_wrapper->construct_ds_desc(dsv_desc);
    m_pso_wrapper->construct();

    vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/cube_vs.hlsl";
    pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/05_pbr/shaders/cube_ps.hlsl";

    m_pso_wrapper_cube = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_pso_wrapper_cube->construct_input_layout(input_layout, _countof(input_layout));
    m_pso_wrapper_cube->construct_root_signature(root_parameters, _countof(root_parameters), nullptr, 0);
    m_pso_wrapper_cube->construct_rt_formats(rtv_formats);
    m_pso_wrapper_cube->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_pso_wrapper_cube->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_pso_wrapper_cube->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_pso_wrapper_cube->construct_ds_format(DXGI_FORMAT_D32_FLOAT);
    m_pso_wrapper_cube->construct_ds_desc(dsv_desc);
    m_pso_wrapper_cube->construct();
}

}
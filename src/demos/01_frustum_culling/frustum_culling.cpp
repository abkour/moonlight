#include "frustum_culling.hpp"

#include <chrono>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace moonlight {

template<typename T>
T* temp_address(T&& rval)
{
    return &rval;
}

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
} scene_pipeline_state_stream;

static struct QuadPipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
} quad_pipeline_state_stream;

struct VertexFormat
{
    XMFLOAT3 p; 
    XMFLOAT2 t;
};

static float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

static float quad_vertices[] =
{
    -1.f, -1.f, 0.f,    0.f, 0.f,
    1.f, -1.f, 0.f,     1.f, 0.f,
    1.f, 1.f, 0.f,      1.f, 1.f,
    -1.f, -1.f, 0.f,    0.f, 0.f,
    1.f, 1.f, 0.f,      1.f, 1.f,
    -1.f, 1.f, 0.f,     0.f, 1.f
};

FrustumCulling::FrustumCulling(HINSTANCE hinstance)
    : IApplication(hinstance)
    , app_initialized(false)
    , fence_value(0)
    , camera(XMFLOAT3(0.f, 0.f, -10.f), XMFLOAT3(0.f, 0.f, 1.f))
    , top_down_camera(XMFLOAT3(0.f, -175.f, -10.f), XMFLOAT3(0.01f, 0.99f, 0.01f))
    , window_width(1600)
    , window_height(800)
{
    text_output.resize(256);

    window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_01_Frustum_Culling",
        window_width,
        window_height,
        &FrustumCulling::WindowMessagingProcess,
        this
    );

    APressed = DPressed = SPressed = WPressed = false;
    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    device                      = _pimpl_create_device(most_sutiable_adapter);
    command_queue               = _pimpl_create_command_queue(device);
    swap_chain                  = _pimpl_create_swap_chain(command_queue, window_width, window_height);
    rtv_descriptor_heap         = _pimpl_create_rtv_descriptor_heap(device, 3);
    quad_rtv_descriptor_heap    = _pimpl_create_rtv_descriptor_heap(device, 2);
    srv_descriptor_heap         = _pimpl_create_srv_descriptor_heap(device, 2);
    dsv_descriptor_heap         = _pimpl_create_dsv_descriptor_heap(device, 2);
    _pimpl_create_backbuffers(device, swap_chain, rtv_descriptor_heap, backbuffers, 3);
    command_allocator           = _pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list_direct         = _pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    fence                       = _pimpl_create_fence(device);
    fence_event                 = _pimpl_create_fence_event();

    scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    viewport0 = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(window_width),
        static_cast<float>(window_height)
    );
    viewport1 = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(window_width) / 2.f,
        static_cast<float>(window_height)
    );
    viewport2 = CD3DX12_VIEWPORT(
        window_width / 2, 
        0.f, 
        static_cast<float>(window_width) / 2.f, 
        static_cast<float>(window_height)
    );

    scene_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
    scene_texture->set_device(
        device.Get(),
        quad_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), 
        srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart()
    );
    scene_texture->set_clear_color(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 0.f));
    scene_texture->init(window_width, window_height);

    // For orthographic view texture
    auto rtv_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto srv_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE ortho_rtv_handle(quad_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE ortho_srv_handle(srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    ortho_rtv_handle.Offset(rtv_size);
    ortho_srv_handle.Offset(srv_size);

    ortho_scene_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
    ortho_scene_texture->set_device(
        device.Get(),
        ortho_rtv_handle,
        ortho_srv_handle
    );
    ortho_scene_texture->set_clear_color(DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f));
    ortho_scene_texture->init(window_width, window_height);

    load_assets();
    UINT dsv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    depth_buffer = _pimpl_create_dsv(device, dsv_handle, window_width, window_height);
    dsv_handle.Offset(dsv_inc_size);
    depth_buffer2 = _pimpl_create_dsv(device, dsv_handle, window_width, window_height);

    command_list_direct->Close();

    // Execute eall command now
    ID3D12CommandList* const command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->ExecuteCommandLists(1, command_lists);
    command_queue_signal(++fence_value);
    wait_for_fence(fence_value);

    app_initialized = true;
}

FrustumCulling::~FrustumCulling()
{
    m_graphicsMemory.reset();
    sprite_batch.reset();
    sprite_font.reset();
    font_descriptor_heap.reset();
}

bool FrustumCulling::is_application_initialized()
{
    return app_initialized;
}

void FrustumCulling::flush()
{
    flush_command_queue();
}

void FrustumCulling::initialize_raw_input_devices()
{
    RAWINPUTDEVICE rids[1];
    rids[0].usUsagePage = 0x01;
    rids[0].usUsage = 0x02;
    rids[0].dwFlags = RIDEV_NOLEGACY;
    rids[0].hwndTarget = 0;

    ThrowIfFailed(RegisterRawInputDevices(rids, 1, sizeof(rids[0])));
}

void FrustumCulling::on_key_down(WPARAM wparam)
{
    switch (wparam)
    {
    case 'A':
        APressed = true;
        break;
    case 'D':
        DPressed = true;
        break;
    case 'S':
        SPressed = true;
        break;
    case 'W':
        WPressed = true;
        break;
    default:
        break;
    }
}

void FrustumCulling::on_key_up(WPARAM wparam)
{
    switch (wparam)
    {
    case 'A':
        APressed = false;
        break;
    case 'D':
        DPressed = false;
        break;
    case 'S':
        SPressed = false;
        break;
    case 'W':
        WPressed = false;
        break;
    default:
        break;
    }
}

void FrustumCulling::on_mouse_move(LPARAM lparam)
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
        camera.rotate(-raw->data.mouse.lLastX, raw->data.mouse.lLastY);
    }

    delete[] lpb;
}

void FrustumCulling::render()
{
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    uint8_t backbuffer_idx = swap_chain->GetCurrentBackBufferIndex();
    auto backbuffer = backbuffers[backbuffer_idx];

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle2(dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    UINT dsv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    dsv_handle2.Offset(dsv_inc_size);

    CD3DX12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle(rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    UINT rtv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Clear
    {
        transition_resource(
            command_list_direct,
            backbuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.1f, 0.1f, 0.1f, 1.f };
        command_list_direct->ClearRenderTargetView(
            backbuffer_rtv_handle.Offset(rtv_inc_size * backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        command_list_direct->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
        command_list_direct->ClearDepthStencilView(dsv_handle2, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
    }

    // Transition SRV to RTV
    scene_texture->transition_to_write_state(command_list_direct.Get());
    auto rt_descriptor = scene_texture->get_rtv_descriptor();
    scene_texture->clear(command_list_direct.Get());
    //
    // Record Scene
    command_list_direct->SetPipelineState(scene_pso.Get());
    command_list_direct->SetGraphicsRootSignature(scene_root_signature.Get());
    command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VERTEX_BUFFER_VIEW vb_views[] =
    {
        vertex_buffer_view,
        instance_id_buffer_view,
        instance_data_buffer_view
    };
    command_list_direct->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list_direct->RSSetViewports(1, &viewport0);
    command_list_direct->RSSetScissorRects(1, &scissor_rect);
    command_list_direct->OMSetRenderTargets(1, &rt_descriptor, FALSE, &dsv_handle);
    command_list_direct->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
    command_list_direct->DrawInstanced(sizeof(vertices) / sizeof(VertexFormat), n_visible_instances, 0, 0);
    // Transition RTV to SRV
    scene_texture->transition_to_read_state(command_list_direct.Get());

    //
    // Render the scene texture to the first viewport
    command_list_direct->SetPipelineState(quad_pso.Get());
    command_list_direct->SetGraphicsRootSignature(quad_root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { srv_descriptor_heap.Get() };
    command_list_direct->SetDescriptorHeaps(1, heaps);
    //srv_gpu_handle.Offset();
    command_list_direct->SetGraphicsRootDescriptorTable(
        0,
        srv_descriptor_heap->GetGPUDescriptorHandleForHeapStart()
    );

    command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list_direct->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
    command_list_direct->RSSetScissorRects(1, &scissor_rect);
    command_list_direct->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, NULL);
    command_list_direct->DrawInstanced(sizeof(quad_vertices) / sizeof(VertexFormat), 1, 0, 0);

    //
    // Render the glyphs
    command_list_direct->RSSetViewports(1, &viewport0);
    ID3D12DescriptorHeap* font_heaps[] = { font_descriptor_heap->Heap() };
    command_list_direct->SetDescriptorHeaps(1, font_heaps);
    
    sprite_batch->Begin(command_list_direct.Get());
    
    //const wchar_t* text_output = L"This is the output";
    XMFLOAT2 origin;
    XMStoreFloat2(&origin, sprite_font->MeasureString(text_output.c_str()) / 2.f);

    sprite_font->DrawString(sprite_batch.get(), text_output.c_str(), font_pos, Colors::White, 0.f, origin);
    sprite_batch->End();

    // Present
    {
        transition_resource(
            command_list_direct,
            backbuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        command_list_direct->Close();
        ID3D12CommandList* const command_lists[] =
        {
            command_list_direct.Get()
        };

        command_queue->ExecuteCommandLists(1, command_lists);

        command_queue_signal(++fence_value);
        swap_chain->Present(1, 0);
        wait_for_fence(fence_value);

        m_graphicsMemory->Commit(command_queue.Get());
    }
}

void FrustumCulling::resize()
{
}

void FrustumCulling::update()
{
    static uint32_t update_count = 0;
    static float elapsed_time_at_threshold = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;

    float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
    const float scale_factor = 1.f;
    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 1000.f;
    XMMATRIX model_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    uint32_t keycode = 0;
    keycode += (uint32_t)WPressed << 3;
    keycode += (uint32_t)SPressed << 2;
    keycode += (uint32_t)DPressed << 1;
    keycode += (uint32_t)APressed;
    auto pre_pos = camera.get_position();
    camera.translate(keycode, elapsed_time);
    auto post_pos = camera.get_position();
    XMFLOAT3 delta_pos(
        post_pos.x - pre_pos.x, 0.f, post_pos.z - pre_pos.z
    );
    XMVECTOR delta_pos_xmv = XMLoadFloat3(&delta_pos);
    top_down_camera.translate(delta_pos_xmv);

    mvp_matrix = XMMatrixMultiply(model_matrix, camera.view);
    mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);

    XMMATRIX projection_matrix_2 = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        0.1,
        200.f
    );

    const float scale_factor_2 = 1.5f;
    model_matrix = XMMatrixScaling(scale_factor_2, scale_factor_2, scale_factor_2);
    mvp_matrix_v2 = XMMatrixMultiply(model_matrix, top_down_camera.view);
    mvp_matrix_v2 = XMMatrixMultiply(mvp_matrix_v2, projection_matrix_2);

    // Ensure that the GPU has finished work, before we render the next frame.
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    // Construct the viewing frustum
    XMFLOAT3 camera_position = camera.get_position();
    XMFLOAT3 camera_direction = camera.get_direction();
    
    Frustum frustum = construct_frustum(
        *reinterpret_cast<Vector3*>(&camera_position),
        *reinterpret_cast<Vector3*>(&camera_direction),
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    auto float_set = [](float* arr, float val, int size) 
    {
        for (int i = 0; i < size; ++i)
        {
            arr[i] = val;
        }
    };
    
    alignas(16) FrustumSIMD frustum_avx2;
    float_set(frustum_avx2.normals[0].nx, frustum.near.normal.x, 8);
    float_set(frustum_avx2.normals[0].ny, frustum.near.normal.y, 8);
    float_set(frustum_avx2.normals[0].nz, frustum.near.normal.z, 8);
    float_set(frustum_avx2.normals[1].nx, frustum.far.normal.x, 8);
    float_set(frustum_avx2.normals[1].ny, frustum.far.normal.y, 8);
    float_set(frustum_avx2.normals[1].nz, frustum.far.normal.z, 8);
    float_set(frustum_avx2.normals[2].nx, frustum.left.normal.x, 8);
    float_set(frustum_avx2.normals[2].ny, frustum.left.normal.y, 8);
    float_set(frustum_avx2.normals[2].nz, frustum.left.normal.z, 8);
    float_set(frustum_avx2.normals[3].nx, frustum.right.normal.x, 8);
    float_set(frustum_avx2.normals[3].ny, frustum.right.normal.y, 8);
    float_set(frustum_avx2.normals[3].nz, frustum.right.normal.z, 8);
    float_set(frustum_avx2.normals[4].nx, frustum.bottom.normal.x, 8);
    float_set(frustum_avx2.normals[4].ny, frustum.bottom.normal.y, 8);
    float_set(frustum_avx2.normals[4].nz, frustum.bottom.normal.z, 8);
    float_set(frustum_avx2.normals[5].nx, frustum.top.normal.x, 8);
    float_set(frustum_avx2.normals[5].ny, frustum.top.normal.y, 8);
    float_set(frustum_avx2.normals[5].nz, frustum.top.normal.z, 8);

    alignas(16) FrustumSIMD abs_frustum_avx2;
    float_set(abs_frustum_avx2.normals[0].nx, std::abs(frustum.near.normal.x), 8);
    float_set(abs_frustum_avx2.normals[0].ny, std::abs(frustum.near.normal.y), 8);
    float_set(abs_frustum_avx2.normals[0].nz, std::abs(frustum.near.normal.z), 8);
    float_set(abs_frustum_avx2.normals[1].nx, std::abs(frustum.far.normal.x), 8);
    float_set(abs_frustum_avx2.normals[1].ny, std::abs(frustum.far.normal.y), 8);
    float_set(abs_frustum_avx2.normals[1].nz, std::abs(frustum.far.normal.z), 8);
    float_set(abs_frustum_avx2.normals[2].nx, std::abs(frustum.left.normal.x), 8);
    float_set(abs_frustum_avx2.normals[2].ny, std::abs(frustum.left.normal.y), 8);
    float_set(abs_frustum_avx2.normals[2].nz, std::abs(frustum.left.normal.z), 8);
    float_set(abs_frustum_avx2.normals[3].nx, std::abs(frustum.right.normal.x), 8);
    float_set(abs_frustum_avx2.normals[3].ny, std::abs(frustum.right.normal.y), 8);
    float_set(abs_frustum_avx2.normals[3].nz, std::abs(frustum.right.normal.z), 8);
    float_set(abs_frustum_avx2.normals[4].nx, std::abs(frustum.bottom.normal.x), 8);
    float_set(abs_frustum_avx2.normals[4].ny, std::abs(frustum.bottom.normal.y), 8);
    float_set(abs_frustum_avx2.normals[4].nz, std::abs(frustum.bottom.normal.z), 8);
    float_set(abs_frustum_avx2.normals[5].nx, std::abs(frustum.top.normal.x), 8);
    float_set(abs_frustum_avx2.normals[5].ny, std::abs(frustum.top.normal.y), 8);
    float_set(abs_frustum_avx2.normals[5].nz, std::abs(frustum.top.normal.z), 8);

    // Change the color of the box to yellow, if it doesn't intersect the frustum
    uint32_t n_culled_objects = 0;
    uint32_t j = 0;
    
    float d[6];
    auto cull_t0 = std::chrono::high_resolution_clock::now();
    const Plane* planes = (Plane*)&frustum;
    for (int i = 0; i < 6; ++i)
    {
        d[i] = dot(planes[i].normal, planes[i].point);
    }
    alignas(16) float d_avx2[48];
    float_set(&d_avx2[0], d[0], 8);
    float_set(&d_avx2[8], d[1], 8);
    float_set(&d_avx2[16], d[2], 8);
    float_set(&d_avx2[24], d[3], 8);
    float_set(&d_avx2[32], d[4], 8);
    float_set(&d_avx2[40], d[5], 8);
    
    for (int i = 0; i < n_instances / 8; ++i)
    {
        uint8_t mask = frustum_contains_aabb_avx2(&frustum_avx2, &abs_frustum_avx2, aabbs[i], d_avx2);
        for (int k = 0; k < 8; ++k)
        {
            if (mask & (0x01 << k))
            {
                int idx = i * 8 + k;
                instance_vertex_offsets[j].displacement =
                    copy_instance_vertex_offsets[idx].displacement;
                ++j;
            } else
            {
                n_culled_objects++;
            }
        }
    }
    auto cull_t1 = std::chrono::high_resolution_clock::now();
    auto cull_time = (cull_t1 - cull_t0).count() * 1e-6;

    n_visible_instances = n_instances - n_culled_objects;
    {
        D3D12_SUBRESOURCE_DATA data_desc = {};
        data_desc.pData = instance_vertex_offsets.get();
        data_desc.RowPitch = sizeof(InstanceDataFormat) * n_visible_instances;
        data_desc.SlicePitch = sizeof(InstanceDataFormat) * n_visible_instances;

        transition_resource(
            command_list_direct,
            instance_data_buffer.Get(),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            D3D12_RESOURCE_STATE_COPY_DEST
        );

        UpdateSubresources(
            command_list_direct.Get(),
            instance_data_buffer.Get(),
            instance_data_intermediate_resource.Get(),
            0, 0, 1, &data_desc
        );

        transition_resource(
            command_list_direct,
            instance_id_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );
    }

    text_output.resize(128);
    swprintf(
        text_output.data(), 
        L"Frame time: %dms\nCull time: %dms\nNumber of cubes: %d\nCulled: %d\nVertices: %d", 
        static_cast<uint32_t>(elapsed_time_at_threshold * 1e3),
        static_cast<uint32_t>(cull_time),
        n_instances,
        n_culled_objects,
        n_culled_objects * 36
    );

    command_list_direct->Close();

    // Execute eall command now
    ID3D12CommandList* const command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->ExecuteCommandLists(1, command_lists);
    command_queue_signal(++fence_value);
    wait_for_fence(fence_value);

    update_count++;
    if (update_count % 25 == 0)
    {
        elapsed_time_at_threshold = elapsed_time;
    }
}

void FrustumCulling::load_assets()
{
    construct_scene();
    initialize_font_rendering();
    load_scene_shader_assets();
    load_quad_shader_assets();
    construct_aabbs_avx2();
}

void FrustumCulling::initialize_font_rendering()
{
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device.Get());
    
    //
    // Initialize descriptor heap for font
    {
        font_descriptor_heap = std::make_unique<DescriptorHeap>(
            device.Get(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            Descriptors::Count
        );
    }

    //
    // Set up the SpriteBatch object
    {
        ResourceUploadBatch resource_upload(device.Get());
        resource_upload.Begin();

        RenderTargetState rt_state(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
        SpriteBatchPipelineStateDescription pps(rt_state);

        sprite_batch = std::make_unique<SpriteBatch>(device.Get(), resource_upload, pps);;

        auto upload_resource_finished = resource_upload.End(command_queue.Get());
        upload_resource_finished.wait();

        sprite_batch->SetViewport(viewport0);
        font_pos.x = static_cast<float>(window_width) * 0.9;
        font_pos.y = static_cast<float>(window_height) * 0.5;
    }

    //
    // Set up the SpriteFont object
    {
        ResourceUploadBatch resource_upload(device.Get());
        resource_upload.Begin();

        std::wstring filename = std::wstring(ROOT_DIRECTORY_WIDE) + L"/rsc/myfile.spriteFont";
        sprite_font = std::make_unique<SpriteFont>(
            device.Get(),
            resource_upload,
            filename.c_str(),
            font_descriptor_heap->GetCpuHandle(Descriptors::CourierFont),
            font_descriptor_heap->GetGpuHandle(Descriptors::CourierFont)
        );

        auto upload_resource_finished = resource_upload.End(command_queue.Get());
        upload_resource_finished.wait();
    }
}

void FrustumCulling::load_scene_shader_assets()
{
    // Vertex buffer uploading
    {
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices))),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&vertex_buffer)
        ));

        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices))),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertex_intermediate_resource)
        ));

        D3D12_SUBRESOURCE_DATA data_desc = {};
        data_desc.pData = vertices;
        data_desc.RowPitch = sizeof(vertices);
        data_desc.SlicePitch = sizeof(vertices);

        UpdateSubresources(
            command_list_direct.Get(), 
            vertex_buffer.Get(), 
            vertex_intermediate_resource.Get(), 
            0, 0, 1, &data_desc
        );

        transition_resource(
            command_list_direct,
            vertex_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
        vertex_buffer_view.SizeInBytes = sizeof(vertices);
        vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    // Instancing id buffer
    
    {
        // Instance vertex buffer
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * n_instances)),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&instance_id_buffer)
        ));

        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * n_instances)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&instance_ids_intermediate_resource)
        ));

        D3D12_SUBRESOURCE_DATA data_desc = {};
        data_desc.pData = instance_ids.get();
        data_desc.RowPitch = sizeof(UINT) * n_instances;
        data_desc.SlicePitch = sizeof(UINT) * n_instances;

        UpdateSubresources(
            command_list_direct.Get(),
            instance_id_buffer.Get(),
            instance_ids_intermediate_resource.Get(),
            0, 0, 1, &data_desc
        );

        transition_resource(
            command_list_direct,
            instance_id_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        instance_id_buffer_view.BufferLocation = instance_id_buffer->GetGPUVirtualAddress();
        instance_id_buffer_view.SizeInBytes = sizeof(UINT) * n_instances;
        instance_id_buffer_view.StrideInBytes = sizeof(UINT);
    }

    // Instance data buffer
    {
        // Instance vertex buffer
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(InstanceDataFormat) * n_instances)),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&instance_data_buffer)
        ));

        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(InstanceDataFormat)* n_instances)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&instance_data_intermediate_resource)
        ));

        D3D12_SUBRESOURCE_DATA data_desc = {};
        data_desc.pData = instance_vertex_offsets.get();
        data_desc.RowPitch = sizeof(InstanceDataFormat) * n_instances;
        data_desc.SlicePitch = sizeof(InstanceDataFormat) * n_instances;

        UpdateSubresources(
            command_list_direct.Get(),
            instance_data_buffer.Get(),
            instance_data_intermediate_resource.Get(),
            0, 0, 1, &data_desc
        );

        transition_resource(
            command_list_direct,
            instance_data_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        instance_data_buffer_view.BufferLocation = instance_data_buffer->GetGPUVirtualAddress();
        instance_data_buffer_view.SizeInBytes = sizeof(InstanceDataFormat) * n_instances;
        instance_data_buffer_view.StrideInBytes = sizeof(InstanceDataFormat);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//instanced_cube_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//instanced_cube_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 1, D3D12_APPEND_ALIGNED_ELEMENT ,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1
        },
        {
            "POSITION", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT ,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1
        },
        {
            "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT ,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1
        }
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsConstantBufferView(1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(2, root_parameters, 0, nullptr, root_signature_flags);

    ComPtr<ID3DBlob> root_signature_blob;
    // TODO: What is the error blob=
    ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &root_signature_desc,
        feature_data.HighestVersion,
        &root_signature_blob,
        &error_blob
    ));

    ThrowIfFailed(device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&scene_root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    scene_pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&scene_pso)));
}

void FrustumCulling::load_quad_shader_assets()
{
    // Quad vertex buffer
    {
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(quad_vertices))),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&quad_vertex_buffer)
        ));

        UINT32* vertex_data_begin = nullptr;
        CD3DX12_RANGE read_range(0, 0);
        ThrowIfFailed(quad_vertex_buffer->Map(
            0,
            &read_range,
            reinterpret_cast<void**>(&vertex_data_begin)
        ));
        memcpy(vertex_data_begin, quad_vertices, sizeof(quad_vertices));
        quad_vertex_buffer->Unmap(0, nullptr);

        quad_vertex_buffer_view.BufferLocation = quad_vertex_buffer->GetGPUVirtualAddress();
        quad_vertex_buffer_view.SizeInBytes = sizeof(quad_vertices);
        quad_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//fullquad_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//fullquad_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init_1_1(1, root_parameters, 1, samplers, root_signature_flags);

    ComPtr<ID3DBlob> root_signature_blob;
    ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &root_signature_desc,
        feature_data.HighestVersion,
        &root_signature_blob,
        &error_blob
    ));

    ThrowIfFailed(device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&quad_root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    quad_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    quad_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    quad_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    quad_pipeline_state_stream.root_signature = quad_root_signature.Get();
    quad_pipeline_state_stream.rs = rasterizer_desc;
    quad_pipeline_state_stream.rtv_formats = rtv_formats;
    quad_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(QuadPipelineStateStream),
        &quad_pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&quad_pso)));
}

void FrustumCulling::construct_scene()
{
    constexpr float xdim = 500.f;
    constexpr float zdim = 500.f;
    constexpr int cubes_per_row = 1000;
    constexpr int cubes_per_column = 1000;
    constexpr float xdelta = xdim / cubes_per_row;
    constexpr float zdelta = zdim / cubes_per_column;

    n_instances = cubes_per_row * cubes_per_column;
    copy_instance_vertex_offsets = std::make_unique<InstanceDataFormat[]>(n_instances);
    instance_vertex_offsets = std::make_unique<InstanceDataFormat[]>(n_instances);
    instance_ids = std::make_unique<UINT[]>(n_instances);

    constexpr float ypos = 0.f;
    float xpos = -xdim / 2.f;
    for (int x = 0; x < cubes_per_column; ++x)
    {
        float zpos = -zdim / 2.f;
        for (int z = 0; z < cubes_per_row; ++z)
        {
            int idx = x * cubes_per_column + z;
            instance_vertex_offsets[idx].displacement = XMFLOAT4(
                xpos, ypos, zpos, 0.f
            );
            instance_vertex_offsets[idx].color = XMFLOAT4(0.f, 1.f, 0.f, 1.f);
            instance_ids[idx] = idx;

            zpos += zdelta;
        }

        xpos += xdelta;
    }

    // Create a copy of the instance data, for use in frustum culling (see update() function)
    memcpy(
        copy_instance_vertex_offsets.get(), 
        instance_vertex_offsets.get(), 
        sizeof(InstanceDataFormat) * n_instances
    );
}

/*
void FrustumCulling::construct_aabbs()
{
    const uint32_t n_vertices = sizeof(vertices) / sizeof(VertexFormat);
    
    // Prepare the data first, magical/phantastical numbers! (do you write that word with ph or f, not sure, probably f)
    float intermediate_buffer[n_vertices * 3];
    for (int i = 0; i < n_vertices; ++i)
    {
        intermediate_buffer[i * 3] = vertices[i * 5];
        intermediate_buffer[i * 3 + 1] = vertices[i * 5 + 1];
        intermediate_buffer[i * 3 + 2] = vertices[i * 5 + 2];
    }
    
    AABB os_aabb = construct_aabb_from_points(
        reinterpret_cast<Vector3*>(intermediate_buffer),
        n_vertices
    );

    for (int j = 0; j < n_instances; ++j)
    {
        AABB new_aabb;
        new_aabb.bmin = os_aabb.bmin;
        new_aabb.bmax = os_aabb.bmax;
        new_aabb.bmin += *(Vector3*)&instance_vertex_offsets[j].displacement;
        new_aabb.bmax += *(Vector3*)&instance_vertex_offsets[j].displacement;
        aabbs.emplace_back(new_aabb);
#ifdef _DEBUG
        char buffer[512];
        sprintf_s(buffer, "bmin: (%f, %f, %f)\t\tbmax: (%f, %f, %f)\n",
            aabbs.back().bmin.x,
            aabbs.back().bmin.y,
            aabbs.back().bmin.z,
            aabbs.back().bmax.x,
            aabbs.back().bmax.y,
            aabbs.back().bmax.z
        );
        OutputDebugStringA(buffer);
#endif
    }
}
*/

void FrustumCulling::construct_aabbs_avx2()
{
    const uint32_t n_vertices = sizeof(vertices) / sizeof(VertexFormat);

    // Prepare the data first, magical/phantastical numbers! (do you write that word with ph or f, not sure, probably f)
    float intermediate_buffer[n_vertices * 3];
    for (int i = 0; i < n_vertices; ++i)
    {
        intermediate_buffer[i * 3] = vertices[i * 5];
        intermediate_buffer[i * 3 + 1] = vertices[i * 5 + 1];
        intermediate_buffer[i * 3 + 2] = vertices[i * 5 + 2];
    }

    AABB os_aabb = construct_aabb_from_points(
        reinterpret_cast<Vector3*>(intermediate_buffer),
        n_vertices
    );

    aabbs.reserve(n_instances / 8);

    for (int j = 0; j < n_instances / 8; ++j)
    {
        AABBSIMD aabb;
        for (int k = 0; k < 8; ++k)
        {
            int i = j * 8 + k;
            aabb.bmin_x[k] = os_aabb.bmin.x + instance_vertex_offsets[i].displacement.x;
            aabb.bmin_y[k] = os_aabb.bmin.y + instance_vertex_offsets[i].displacement.y;
            aabb.bmin_z[k] = os_aabb.bmin.z + instance_vertex_offsets[i].displacement.z;
            aabb.bmax_x[k] = os_aabb.bmax.x + instance_vertex_offsets[i].displacement.x;
            aabb.bmax_y[k] = os_aabb.bmax.y + instance_vertex_offsets[i].displacement.y;
            aabb.bmax_z[k] = os_aabb.bmax.z + instance_vertex_offsets[i].displacement.z;
        }
        aabbs.push_back(aabb);
    }
}

void FrustumCulling::transition_resource(
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES before_state,
    D3D12_RESOURCE_STATES after_state)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        before_state,
        after_state
    );

    command_list->ResourceBarrier(1, &barrier);
}

void FrustumCulling::command_queue_signal(uint64_t fence_value)
{
    ThrowIfFailed(command_queue->Signal(fence.Get(), fence_value));
}

void FrustumCulling::flush_command_queue()
{
    ++fence_value;
    command_queue_signal(fence_value);
    wait_for_fence(fence_value);
}

void FrustumCulling::wait_for_fence(uint64_t fence_value)
{
    if (fence->GetCompletedValue() < fence_value)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
        WaitForSingleObject(fence_event, INFINITE);
    }
}

}
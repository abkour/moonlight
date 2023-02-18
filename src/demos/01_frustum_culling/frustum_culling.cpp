#include "frustum_culling.hpp"

#include <chrono>
#include <numeric>  // for std::iota

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

static float interleaved_cube_vertices[] = {
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

static float interleaved_quad_vertices[] =
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
    , camera(XMFLOAT3(0.f, 0.f, -10.f), XMFLOAT3(0.f, 0.f, 1.f), 500.f)
{
    text_output.resize(256);

    window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_01_Frustum_Culling",
        1600,
        800,
        &FrustumCulling::WindowMessagingProcess,
        this
    );

    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    device                      = _pimpl_create_device(most_sutiable_adapter);
    command_queue               = _pimpl_create_command_queue(device);
    swap_chain                  = _pimpl_create_swap_chain(command_queue, window->width(), window->height());
    command_allocator           = _pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list_direct         = _pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    fence                       = _pimpl_create_fence(device);
    fence_event                 = _pimpl_create_fence_event();

    rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3
    );
    quad_rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2
    );
    dsv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2
    );
    srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2
    );
    vs_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2
    );

    _pimpl_create_backbuffers(device, swap_chain, rtv_descriptor_heap->get_underlying(), backbuffers, 3);


    scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    viewport0 = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(window->width()),
        static_cast<float>(window->height())
    );

    scene_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
    scene_texture->set_device(
        device.Get(),
        quad_rtv_descriptor_heap->cpu_handle(), 
        srv_descriptor_heap->cpu_handle()
    );
    scene_texture->set_clear_color(DirectX::XMFLOAT4(0.1f, 0.1f, 0.1f, 0.f));
    scene_texture->init(window->width(), window->height());

    load_assets();
    depth_buffer = _pimpl_create_dsv(
        device, 
        dsv_descriptor_heap->cpu_handle(), 
        window->width(), window->height()
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

void FrustumCulling::on_key_event(const PackedKeyArguments key_state)
{
    switch (key_state.key_state)
    {
    case PackedKeyArguments::Released:
        keyboard_state.reset(key_state.key);
        break;
    case PackedKeyArguments::Pressed:
        keyboard_state.set(key_state.key);
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

void FrustumCulling::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = swap_chain->GetCurrentBackBufferIndex();
    auto backbuffer = backbuffers[backbuffer_idx];
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_descriptor_heap->cpu_handle();
    UINT rtv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle = rtv_descriptor_heap->cpu_handle(backbuffer_idx);
    // Transition SRV to RTV
    scene_texture->transition_to_write_state(command_list);
    auto rt_descriptor = scene_texture->get_rtv_descriptor();
    scene_texture->clear(command_list);
    //
    // Render Scene to Texture
    command_list->SetPipelineState(scene_pso.Get());
    command_list->SetGraphicsRootSignature(scene_root_signature.Get());
    command_list->SetGraphicsRootShaderResourceView(1, instance_id_buffer->gpu_virtual_address());
    command_list->SetGraphicsRootShaderResourceView(2, instance_data_buffer->gpu_virtual_address());
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &viewport0);
    command_list->RSSetScissorRects(1, &scissor_rect);
    command_list->OMSetRenderTargets(1, &rt_descriptor, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
    command_list->DrawInstanced(sizeof(interleaved_cube_vertices) / sizeof(VertexFormat), n_visible_instances, 0, 0);
    // Transition RTV to SRV
    scene_texture->transition_to_read_state(command_list);

    //
    // Render the scene texture to the first viewport
    command_list->SetPipelineState(quad_pso.Get());
    command_list->SetGraphicsRootSignature(quad_root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { srv_descriptor_heap->get_underlying() };
    command_list->SetDescriptorHeaps(1, heaps);
    command_list->SetGraphicsRootDescriptorTable(
        0, srv_descriptor_heap->gpu_handle()
    );
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
    command_list->RSSetScissorRects(1, &scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, NULL);
    command_list->DrawInstanced(sizeof(interleaved_quad_vertices) / sizeof(VertexFormat), 1, 0, 0);

    //
    // Render the glyphs
    command_list->RSSetViewports(1, &viewport0);
    ID3D12DescriptorHeap* font_heaps[] = { font_descriptor_heap->Heap() };
    command_list->SetDescriptorHeaps(1, font_heaps);

    XMFLOAT2 origin;
    XMStoreFloat2(&origin, sprite_font->MeasureString(text_output.c_str()) / 2.f);
    sprite_batch->Begin(command_list);
    sprite_font->DrawString(sprite_batch.get(), text_output.c_str(), font_pos, Colors::White, 0.f, origin);
    sprite_batch->End();
}

void FrustumCulling::render()
{
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    uint8_t backbuffer_idx = swap_chain->GetCurrentBackBufferIndex();
    auto backbuffer = backbuffers[backbuffer_idx];
    // Clear
    {
        transition_resource(
            command_list_direct,
            backbuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.7f, 0.7f, 0.7f, 1.f };
        command_list_direct->ClearRenderTargetView(
            rtv_descriptor_heap->cpu_handle(backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        command_list_direct->ClearDepthStencilView(
            dsv_descriptor_heap->cpu_handle(), 
            D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL
        );
    }

    record_command_list(command_list_direct.Get());

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
    static float cull_time_at_threshold = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;

    float aspect_ratio = static_cast<float>(window->width()) / static_cast<float>(window->height());
    const float scale_factor = 1.f;
    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 2000.f;
    XMMATRIX model_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    camera.translate(keyboard_state, elapsed_time);
    mvp_matrix = XMMatrixMultiply(model_matrix, camera.view);
    mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);

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
    
    FrustumSIMD frustum_avx2;
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

    FrustumSIMD abs_frustum_avx2;
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

    alignas(32) float d_avx2[48];
    auto cull_t0 = std::chrono::high_resolution_clock::now();
    const Plane* planes = (Plane*)&frustum;
    for (int i = 0; i < 6; ++i)
    {
        float_set(&d_avx2[i * 8], dot(planes[i].normal, planes[i].point), 8);
    }
    
    n_visible_instances = 0;
    for (int i = 0; i < n_instances / 8; ++i)
    {
        uint8_t mask = frustum_contains_aabb_avx2(&frustum_avx2, &abs_frustum_avx2, aabbs[i], d_avx2);
        for (int k = 0; k < 8; ++k)
        {
            if (mask & (0x01 << k))
            {
                instance_ids[n_visible_instances] = i * 8 + k;
                ++n_visible_instances;
            }
        }
    }

    auto cull_t1 = std::chrono::high_resolution_clock::now();
    auto cull_time = (cull_t1 - cull_t0).count() * 1e-3;
    // Update the contents of the offset buffer
    instance_id_buffer->update(
        device.Get(), command_list_direct.Get(), 
        instance_ids.get(), sizeof(UINT) * n_visible_instances
    );

    text_output.resize(128);
    uint32_t n_culled_objects = n_instances - n_visible_instances;
    swprintf(
        text_output.data(), 
        L"Frame time: %dms\nCull time: %dus\nNumber of cubes: %d\nCulled: %d\nTriangles: %d", 
        static_cast<uint32_t>(elapsed_time_at_threshold * 1e3),
        static_cast<uint32_t>(cull_time),
        n_instances,
        n_culled_objects,
        n_culled_objects * 36 / 3
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
    if (update_count % 16 == 0)
    {
        cull_time_at_threshold = cull_time;
    }
    if (update_count % 25 == 0)
    {
        elapsed_time_at_threshold = elapsed_time;
    }
}

void FrustumCulling::load_assets()
{
    const float scene_xdim = 700.f;
    const float scene_ydim = 700.f;
    const int n_cubes_per_row = 300;
    const int n_cubes_per_column = 300;
    n_instances = n_cubes_per_row * n_cubes_per_column;
    
    instance_ids = std::make_unique<UINT[]>(n_instances);
    for (int i = 0; i < n_instances; ++i)
    {
        instance_ids[i] = i;
    }
    instance_vertex_offsets = std::make_unique<InstanceAttributes[]>(n_instances);
    construct_scene_of_cubes(
        instance_vertex_offsets.get(),
        scene_xdim, scene_ydim, 
        n_cubes_per_row, n_cubes_per_column
    );
    initialize_font_rendering();
    load_scene_shader_assets();
    load_quad_shader_assets();
    
    const uint32_t n_vertices = sizeof(interleaved_cube_vertices) / sizeof(VertexFormat);
    aabbs.resize(n_instances / 8);
    construct_instanced_aabbs(
        aabbs.data(), 
        n_instances / 8,
        interleaved_cube_vertices,
        sizeof(interleaved_cube_vertices) / sizeof(VertexFormat),
        sizeof(VertexFormat),
        reinterpret_cast<float*>(instance_vertex_offsets.get()),
        sizeof(InstanceAttributes)
    );
}

void FrustumCulling::initialize_font_rendering()
{
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device.Get());
    
    //
    // Initialize descriptor heap for font
    {
        font_descriptor_heap = std::make_unique<::DescriptorHeap>(
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
        font_pos.x = static_cast<float>(window->width()) * 0.9;
        font_pos.y = static_cast<float>(window->height()) * 0.5;
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
        vertex_buffer = std::make_unique<DX12Resource>();
        vertex_buffer->upload(device.Get(), command_list_direct.Get(), 
            interleaved_cube_vertices, sizeof(interleaved_cube_vertices)
        );

        vertex_buffer_view.BufferLocation = vertex_buffer->gpu_virtual_address();
        vertex_buffer_view.SizeInBytes = sizeof(interleaved_cube_vertices);
        vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    // Instancing ID SRV
    {
        instance_id_buffer = std::make_unique<DX12Resource>();
        instance_id_buffer->upload(
            device.Get(), command_list_direct.Get(),
            instance_ids.get(), sizeof(UINT) * n_instances
        );
        
        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = n_instances;
        buffer_desc.StructureByteStride = sizeof(UINT);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(
            instance_id_buffer->get_underlying(),
            &srv_desc,
            vs_srv_descriptor_heap->cpu_handle()
        );
    }

    // Instance data SRV
    {
        instance_data_buffer = std::make_unique<DX12Resource>();
        instance_data_buffer->upload(
            device.Get(), command_list_direct.Get(),
            instance_vertex_offsets.get(), sizeof(InstanceAttributes) * n_instances
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = n_instances;
        buffer_desc.StructureByteStride = sizeof(InstanceAttributes);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(
            instance_data_buffer->get_underlying(),
            &srv_desc,
            vs_srv_descriptor_heap->cpu_handle(1)
        );
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//instanced_cube_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//instanced_cube_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_t(input_layout);

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

    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsShaderResourceView(0);
    root_parameters[2].InitAsShaderResourceView(1);

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
        quad_vertex_buffer = std::make_unique<DX12Resource>();
        quad_vertex_buffer->upload(
            device.Get(), command_list_direct.Get(), 
            interleaved_quad_vertices, sizeof(interleaved_quad_vertices)
        );

        quad_vertex_buffer_view.BufferLocation = quad_vertex_buffer->gpu_virtual_address();
        quad_vertex_buffer_view.SizeInBytes = sizeof(interleaved_quad_vertices);
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
    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_t(input_layout);

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
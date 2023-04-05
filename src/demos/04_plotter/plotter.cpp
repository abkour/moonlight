#include "plotter.hpp"
#include <random>
#include "../../simple_math.hpp"
#include <cmath>
#include "../../utility/random_number.hpp"

using namespace Microsoft::WRL;
using namespace DirectX;

#define SIZEOFQUAD 72

namespace
{

float random_normalized_float()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(mt);
}

float random_in_range(float rlow, float rhigh)
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(rlow, rhigh);
    return dist(mt);
}

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
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
} scene_pipeline_state_stream;

Plotter::Plotter(HINSTANCE hinstance)
    : IApplication(hinstance)
    , m_app_initialized(false)
    , camera(XMFLOAT3(0.f, 0.f, -2.f), XMFLOAT3(0.f, 0.f, 1.f), 2.5f)
{
    m_window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_Template",
        1000,
        1000,
        &Plotter::WindowMessagingProcess,
        this
    );

    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    m_device = _pimpl_create_device(most_sutiable_adapter);
    m_command_queue = std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_allocator = _pimpl_create_command_allocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_list_direct = _pimpl_create_command_list(m_device, m_command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

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

    load_assets();

    m_app_initialized = true;
}

bool Plotter::is_application_initialized()
{
    return m_app_initialized;
}

void Plotter::flush() 
{
    m_command_queue->flush();
}

void Plotter::on_key_event(const PackedKeyArguments key_state) 
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

void Plotter::on_mouse_move(LPARAM lparam) 
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
        camera.rotate(-raw->data.mouse.lLastX, -raw->data.mouse.lLastY);
    }

    delete[] lpb;
}

void Plotter::render() 
{
    m_swap_chain->transition_to_rtv(m_command_list_direct.Get());

    // Clear backbuffer
    const FLOAT clear_color[] = { 0.05f, 0.05f, 0.05f, 1.f };
    m_command_list_direct->ClearRenderTargetView(
        m_swap_chain->backbuffer_rtv_descriptor_handle(
            m_swap_chain->current_backbuffer_index()),
        clear_color,
        0,
        NULL
    );

    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    m_command_list_direct->SetPipelineState(m_scene_pso.Get());
    m_command_list_direct->SetGraphicsRootSignature(m_scene_root_signature.Get());

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    m_command_list_direct->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    m_command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_command_list_direct->RSSetViewports(1, &m_viewport);
    m_command_list_direct->RSSetScissorRects(1, &m_scissor_rect);
    m_command_list_direct->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, nullptr);
    m_command_list_direct->SetGraphicsRoot32BitConstants(0, 16, &mvp_matrix, 0);
    m_command_list_direct->DrawInstanced(m_num_points * 6, 1, 0, 0);

    m_swap_chain->transition_to_present(m_command_list_direct.Get());

    m_command_list_direct->Close();
    ID3D12CommandList* command_lists[] =
    {
        m_command_list_direct.Get()
    };

    m_command_queue->execute_command_list(command_lists, 1);
    m_command_queue->signal();
    m_command_queue->wait_for_fence();
    m_swap_chain->present();

    ThrowIfFailed(m_command_allocator->Reset());
    ThrowIfFailed(m_command_list_direct->Reset(m_command_allocator.Get(), NULL));
}

void Plotter::resize() 
{
    flush();

    m_window->resize();
    m_swap_chain->resize(m_device.Get(), m_window->width(), m_window->height());

    m_viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );
}

void Plotter::update() 
{
    static uint32_t update_count = 0;
    static float elapsed_time_at_threshold = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;

    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());
    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 100.f;
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    static XMFLOAT4 center_f4(0.f, 0.f, 0.f, 0.f);
    static XMVECTOR center = XMLoadFloat4(&center_f4);
    camera.translate(keyboard_state, elapsed_time);
    mvp_matrix = XMMatrixMultiply(camera.view, projection_matrix);
}

void Plotter::initialize_raw_input_devices()
{
    RAWINPUTDEVICE rids[1];
    rids[0].usUsagePage = 0x01;
    rids[0].usUsage = 0x02;
    rids[0].dwFlags = RIDEV_NOLEGACY;
    rids[0].hwndTarget = 0;

    ThrowIfFailed(RegisterRawInputDevices(rids, 1, sizeof(rids[0])));
}

void sample_triangle(float& r0, float& r1)
{
    r0 = random_normalized_float();
    r1 = random_normalized_float();

    float sq = std::sqrt(r0);
    Vector2<float> sample(1 - sq, r1 * sq);
    r0 = sample.x;
    r1 = sample.y;

    r0 -= 0.5f;
    r1 -= 0.5f;
}

void sample_unit_circle(float& r0, float& r1)
{
    // Rejection sampling
    while (true)
    {
        r0 = random_in_range(-1.f, 1.f);
        r1 = random_in_range(-1.f, 1.f);

        float hyp = sqrt(r0 * r0 + r1 * r1);
        if (hyp < 1.f)
            break;
    }

    r0 *= 0.5f;
    r1 *= 0.5f;
}

void sample_square(float& r0, float& r1)
{
    r0 = random_in_range(-0.5f, 0.5f);
    r1 = random_in_range(-0.5f, 0.5f);
}

void sample_rectangle(float& r0, float& r1)
{
    r0 = random_in_range(-0.75f, 0.75f);
    r1 = random_in_range(-0.5f, 0.5f);
}

void sample_unit_circle_inv(float& r0, float& r1)
{
    using vec2f = Vector2<float>;
    using vec3f = Vector3<float>;
    
    r0 = random_in_range(0.f, 1.f);
    r1 = random_in_range(0.f, 1.f);
    const vec2f p(r0, r1);

    vec2f off = 2.f * p - vec2f(1.f);
    if (off.x == 0 && off.y == 0) {
        return;
    }

    float theta, r;
    if (std::abs(off.x) > std::abs(off.y)) {
        r = off.x;
        theta = (ML_PI / 4) * (off.y / off.x);
    } else {
        r = off.y;
        theta = (ML_PI / 2) - (ML_PI / 4) * (off.x / off.y);
    }

    r0 = r * std::cos(theta);
    r1 = r * std::sin(theta);

    r0 *= 0.5f;
    r1 *= 0.5f;
}

void sample_unit_circle_bad0(float& r0, float& r1)
{
    r0 = random_in_range(-0.75, 0.75);
    r1 = random_in_range(-0.75, 0.75);

    float r = r0;

    r0 = r * std::cos(2 * ML_PI * r1);
    r1 = r * std::sin(2 * ML_PI * r1);
}

void sample_unit_circle_bad1(float& r0, float& r1)
{
    r0 = random_in_range(-0.75, 0.75);
    r1 = random_in_range(-0.75, 0.75);

    float r = std::sqrt(r0);

    r0 = r * std::cos(2 * ML_PI * r1);
    r1 = r * std::sin(2 * ML_PI * r1);
}

void sasmple_hemisphere(float& r0, float& r1, float& r2)
{
    r0 = random_in_range(-1.f, 1.f);
    r1 = random_in_range(-1.f, 1.f);
    
    float z = r0;
    float r = std::sqrt(std::max(0.f, 1.f - z * z));
    float phi = 2 * ML_PI * r1;

    r0 = r * std::cos(phi);
    r1 = r * std::sin(phi);
    r2 = z;
}

void sample_sphere(float& r0, float& r1, float& r2)
{
    r0 = random_in_range(-1.f, 1.f);
    r1 = random_in_range(-1.f, 1.f);

    float z = 1 - 2 * r0;
    float r = std::sqrt(std::max(0.f, (float)1 - z * z));
    float phi = 2 * ML_PI * r1;

    r0 = r * std::cos(phi);
    r1 = r * std::sin(phi);
    r2 = z;
}

void sample_cosine_hemisphere(float& r0, float& r1, float& r2) {
    sample_unit_circle_inv(r0, r1);
    float z = std::sqrt(std::max(0.f, 1 - r0 * r0 - r1 * r1));
    r2 = z;
}

void sample_cone(float& r0, float& r1, float& r2) 
{
    r0 = random_in_range(-1.f, 1.f);
    r1 = random_in_range(-1.f, 1.f);
    float cosThetaMax = random_in_range(0.f, 1.f);

    float cosTheta = (1.f - r0) + r0 * cosThetaMax;
    float sinTheta = std::sqrt(1.f - cosTheta * cosTheta);
    float phi = r1 * 2 * ML_PI;
    
    r0 = std::cos(phi) * sinTheta;
    r1 = std::sin(phi) * sinTheta;
    r2 = cosTheta;
}

void Plotter::load_assets()
{
    float dx = 1.f / m_window->width();
    float dy = 1.f / m_window->height();
    float point_scale = 6.f;

    float quad[] =
    {
        0.f, 0.f, 0.f,
        dx, 0.f, 0.f,
        dx, dy, 0.f,
        0.f, 0.f, 0.f,
        dx, dy, 0.f,
        0.f, dy, 0.f
    };

    m_num_points = 10000;
    std::vector<float> vertices;
    vertices.reserve(m_num_points * _countof(quad));

    constexpr int sampling_method = 11;

    for (int i = 0; i < m_num_points; ++i)
    {
        float r0, r1, r2;
        r2 = 0.f;

        switch (sampling_method)
        {
        case 0:
            sample_triangle(r0, r1);
            break;
        case 1:
            sample_unit_circle(r0, r1);
            break;
        case 2:
            sample_square(r0, r1);
            break;
        case 3:
            sample_rectangle(r0, r1);
            break;
        case 4:
            sample_unit_circle_inv(r0, r1);
            break;
        case 5:
            sample_unit_circle_bad0(r0, r1);
            break;
        case 6:
            sample_unit_circle_bad1(r0, r1);
            break;
        case 7:
            sample_sphere(r0, r1, r2);
            break;
        case 8:
            sasmple_hemisphere(r0, r1, r2);
            break;
        case 9:
            sample_cosine_hemisphere(r0, r1, r2);
            break;
        case 10:
            sample_cone(r0, r1, r2);
            break;
        case 11:
        {
            Vector3<float> res = random_cosine_direction();
            r0 = res.x; r1 = res.y; r2 = res.z;
        }
            break;
        default:
            break;
        }
        
        vertices.push_back(r0);
        vertices.push_back(r1);
        vertices.push_back(r2);

        vertices.push_back(r0 + dx * point_scale);
        vertices.push_back(r1);
        vertices.push_back(r2);

        vertices.push_back(r0 + dx * point_scale);
        vertices.push_back(r1 + dy * point_scale);
        vertices.push_back(r2);

        vertices.push_back(r0);
        vertices.push_back(r1);
        vertices.push_back(r2);

        vertices.push_back(r0 + dx * point_scale);
        vertices.push_back(r1 + dy * point_scale);
        vertices.push_back(r2);

        vertices.push_back(r0);
        vertices.push_back(r1 + dy * point_scale);
        vertices.push_back(r2);
    }

    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(),
            vertices.data(),
            vertices.size() * sizeof(float),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = vertices.size() * sizeof(float);
        m_vertex_buffer_view.StrideInBytes = sizeof(float) * 3;
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/04_plotter/passthrough_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/04_plotter/passthrough_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[1] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
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

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?

    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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

    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = m_scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(m_device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&m_scene_pso)));
}

}
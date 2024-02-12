#include "pso.hpp"

#include "d3d12_runtime_dxc.hpp"
#include "../helpers.h"

namespace moonlight
{

using namespace Microsoft::WRL;

void PipelineStateObject::construct_root_signature(
    const CD3DX12_ROOT_PARAMETER1* root_params, const std::size_t num_root_params,
    const CD3DX12_STATIC_SAMPLER_DESC* sampler_descs, const std::size_t num_sampler_descs)
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(m_device->CheckFeatureSupport(
        D3D12_FEATURE_ROOT_SIGNATURE, 
        &feature_data, 
        sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(
        num_root_params, root_params,
        num_sampler_descs, sampler_descs,
        root_signature_flags
    );

    ComPtr<ID3DBlob> root_signature_blob;
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
        IID_PPV_ARGS(&m_root_signature)
    ));

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE(
            m_root_signature.Get()
    ));
}

void PipelineStateObject::construct_input_layout(
    const D3D12_INPUT_ELEMENT_DESC* input_comps,
    const std::size_t num_input_comps)
{
    m_global_pss_field->m_input_element_descriptors.resize(num_input_comps);

    std::memcpy(
        m_global_pss_field->m_input_element_descriptors.data(),
        input_comps,
        sizeof(D3D12_INPUT_ELEMENT_DESC) * num_input_comps
    );

    D3D12_INPUT_LAYOUT_DESC input_layout = {};
    input_layout.NumElements = num_input_comps;
    input_layout.pInputElementDescs = m_global_pss_field->m_input_element_descriptors.data();

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT(input_layout)
    );
}

void PipelineStateObject::construct_vs(
    const wchar_t* vs_path, 
    const wchar_t* entry_point,
    const wchar_t* hlsl_version)
{
    std::unique_ptr<RuntimeDXCCompiler> vs_compiler = std::make_unique<RuntimeDXCCompiler>(
        vs_path,
        entry_point, hlsl_version
    );

    m_global_pss_field->m_blob_vs = vs_compiler->get_code_blob();

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_VS(CD3DX12_SHADER_BYTECODE(m_global_pss_field->m_blob_vs.Get())
    ));
}

void PipelineStateObject::construct_ps(
    const wchar_t* ps_path, 
    const wchar_t* entry_point, 
    const wchar_t* hlsl_version)
{
    std::unique_ptr<RuntimeDXCCompiler> ps_compiler = std::make_unique<RuntimeDXCCompiler>(
        ps_path,
        entry_point, hlsl_version
    );

    m_global_pss_field->m_blob_ps = ps_compiler->get_code_blob();

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_PS(CD3DX12_SHADER_BYTECODE(m_global_pss_field->m_blob_ps.Get())
    ));
}

void PipelineStateObject::construct_rasterizer(
    const D3D12_FILL_MODE fill_mode,
    const D3D12_CULL_MODE cull_mode,
    const BOOL enable_depth_clipping)
{
    m_global_pss_field->m_rasterizer_desc.FillMode = fill_mode;
    m_global_pss_field->m_rasterizer_desc.CullMode = cull_mode;
    m_global_pss_field->m_rasterizer_desc.DepthClipEnable = enable_depth_clipping;
    m_global_pss_field->m_rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER(
            m_global_pss_field->m_rasterizer_desc
    ));
}

void PipelineStateObject::construct_blend_desc(const D3D12_BLEND_DESC& blend_desc)
{
    std::memcpy(
        &m_global_pss_field->m_blend_desc,
        &blend_desc,
        sizeof(D3D12_BLEND_DESC)
    );

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(
            static_cast<CD3DX12_BLEND_DESC>(m_global_pss_field->m_blend_desc)
    ));
}

void PipelineStateObject::construct_ds_desc(const D3D12_DEPTH_STENCIL_DESC& ds_desc)
{
    std::memcpy(
        &m_global_pss_field->m_ds_desc,
        &ds_desc,
        sizeof(D3D12_DEPTH_STENCIL_DESC)
    );

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL(
            static_cast<CD3DX12_DEPTH_STENCIL_DESC>(m_global_pss_field->m_ds_desc)
    ));
}

void PipelineStateObject::construct_ds_format(const DXGI_FORMAT& depth_format)
{
    m_global_pss_field->m_ds_format = depth_format;

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT(
            m_global_pss_field->m_ds_format
    ));
}

void PipelineStateObject::construct_rt_formats(
    const D3D12_RT_FORMAT_ARRAY& rt_format_array)
{
    std::memcpy(
        &m_global_pss_field->m_rt_format,
        &rt_format_array,
        sizeof(D3D12_RT_FORMAT_ARRAY)
    );
    
    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS(
            m_global_pss_field->m_rt_format
    ));
}

void PipelineStateObject::construct_rt_formats(
    const std::initializer_list<DXGI_FORMAT>& rt_formats)
{
    for (int i = 0; const auto& format : rt_formats)
    {
        m_global_pss_field->m_rt_format.RTFormats[i] = format;
        ++i;
    }

    m_global_pss_field->m_rt_format.NumRenderTargets = rt_formats.size();

    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS(
            m_global_pss_field->m_rt_format
    ));
}

void PipelineStateObject::construct(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
    m_pss_desc.attach(
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY(
            type
    ));

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        m_pss_desc.size(),
        m_pss_desc.data()
    };

    ThrowIfFailed(m_device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&m_pso)));
}

}
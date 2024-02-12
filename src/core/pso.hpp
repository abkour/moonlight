#pragma once
#include "../../ext/d3dx12.h"
#include "global_pss_field.hpp"
#include "pss_desc.hpp"

namespace moonlight
{

class PipelineStateObject
{
public:

    PipelineStateObject(
        Microsoft::WRL::ComPtr<ID3D12Device2> device,
        const std::shared_ptr<GlobalPipelineStateStreamField>& pss_field)
        : m_device(device)
        , m_global_pss_field(pss_field)
    {
    }

    void construct_root_signature(
        const CD3DX12_ROOT_PARAMETER1* root_params, 
        const std::size_t num_root_params,
        const CD3DX12_STATIC_SAMPLER_DESC* sampler_descs, 
        const std::size_t num_sampler_descs
    );

    void construct_input_layout(
        const D3D12_INPUT_ELEMENT_DESC* input_comps,
        const std::size_t num_input_comps
    );

    void construct_vs(
        const wchar_t* vs_path, 
        const wchar_t* entry_point,
        const wchar_t* hlsl_version
    );
    
    void construct_ps(
        const wchar_t* ps_path,
        const wchar_t* entry_point,
        const wchar_t* hlsl_version
    );

    void construct_rasterizer(
        const D3D12_FILL_MODE fill_mode,
        const D3D12_CULL_MODE cull_mode,
        const BOOL enable_depth_clipping = TRUE
    );

    void construct_blend_desc(const D3D12_BLEND_DESC& blend_desc);

    void construct_ds_desc(const D3D12_DEPTH_STENCIL_DESC& ds_desc);

    void construct_ds_format(const DXGI_FORMAT& depth_format);

    void construct_rt_formats(const D3D12_RT_FORMAT_ARRAY& rt_format_array);

    void construct_rt_formats(const std::initializer_list<DXGI_FORMAT>& rt_formats);

    void construct(D3D12_PRIMITIVE_TOPOLOGY_TYPE type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    ID3D12RootSignature* root_signature()
    {
        return m_root_signature.Get();
    }
    
    ID3D12PipelineState* pso()
    {
        return m_pso.Get();
    }

private:

    std::shared_ptr<GlobalPipelineStateStreamField> m_global_pss_field;

    Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

    PipelineStateStreamDesc m_pss_desc;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
};

}
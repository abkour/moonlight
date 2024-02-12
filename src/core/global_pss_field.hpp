#pragma once

#include "../../ext/d3dx12.h"
#include <vector>

namespace moonlight
{

struct GlobalPipelineStateStreamField
{
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_as;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_ms;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_vs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_gs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_hs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_ds;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_ps;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_cs;

    CD3DX12_RASTERIZER_DESC m_rasterizer_desc;

    D3D12_PIPELINE_STATE_FLAGS      m_flags;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE   m_primitive_topology_type;
    D3D12_RT_FORMAT_ARRAY           m_rt_format;
    D3D12_DEPTH_STENCIL_DESC        m_ds_desc;
    D3D12_BLEND_DESC                m_blend_desc;

    DXGI_FORMAT m_ds_format;
    
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_element_descriptors;
};

}
#pragma once

#include "../../ext/d3dx12.h"
#include <vector>

namespace moonlight
{

struct GlobalPipelineStateStreamField
{
    D3D12_PIPELINE_STATE_FLAGS m_flags;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_as;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_ms;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_vs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_gs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_hs;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_ds;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_ps;
    Microsoft::WRL::ComPtr<ID3DBlob> m_blob_cs;

    DXGI_FORMAT m_ds_format;
    D3D12_RT_FORMAT_ARRAY m_rt_format;

    CD3DX12_RASTERIZER_DESC m_rasterizer_desc;

    D3D12_PRIMITIVE_TOPOLOGY_TYPE m_primitive_topology_type;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_element_descriptors;
};

}
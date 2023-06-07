#pragma once
#include "../../ext/d3dx12.h"
#include <cassert>
#include <memory>

namespace moonlight
{

constexpr std::size_t AGGERGATE_PSS_SUBOJECT_SIZE =
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_FLAGS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_NODE_MASK) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_VS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_GS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_STREAM_OUTPUT) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_HS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_DS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_PS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_AS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_MS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_CS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO) +
    sizeof(CD3DX12_PIPELINE_STATE_STREAM_VIEW_INSTANCING);

class PipelineStateStreamDesc
{
public:
    
    PipelineStateStreamDesc()
    {
        m_byte_stream = std::make_unique<uint8_t[]>(AGGERGATE_PSS_SUBOJECT_SIZE);
        m_cursor = 0;

        assert((std::size_t)m_byte_stream.get() % sizeof(void*) == 0, "Error, misaligned memory boundary");
    }

    template<
        typename InnerStructType, 
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, 
        typename DefaultArg = InnerStructType>
    void attach(
        const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<
            InnerStructType, Type, DefaultArg>& arg)
    {
        std::memcpy(
            m_byte_stream.get() + m_cursor,
            std::addressof(arg),
            sizeof(arg)
        );

        m_cursor += sizeof(arg);
    }

    uint8_t* data()
    {
        return m_byte_stream.get();
    }

    std::size_t size() const
    {
        return m_cursor;
    }

private:

    std::size_t m_cursor;
    std::unique_ptr<uint8_t[]> m_byte_stream;
};

}
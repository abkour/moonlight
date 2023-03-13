#pragma once
#include "../../ext/d3dx12.h"
#include "../utility/common.hpp"
#include "../helpers.h"
#include "../project_defines.hpp"
#include "d3d12_common.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>

namespace moonlight
{

/*
*   Class purpose:
*   This class describes a shared resource that was externally created.
*   This class has functionality for CPU writes. This is useful when the 
*   same texture can be written to by the CPU and GPU.
*   
*   GPU writes and synchronization are not handled by this class.
*/

class CPUGPUTexture2D
{
public:

    CPUGPUTexture2D(
        ID3D12Resource* texture,
        DXGI_FORMAT format
    );

    void initialize(
        ID3D12Device2* device,
        uint32_t texture_width,
        uint32_t texture_height,
        uint32_t format_size
    );

    void upload(
        ID3D12Device2* device,
        ID3D12GraphicsCommandList* command_list,
        D3D12_RESOURCE_STATES resource_state,
        void* data, 
        unsigned width, unsigned height, unsigned format_size
    );

    void resize(
        ID3D12Device2* device,
        ID3D12Resource* resized_texture,
        unsigned width, unsigned height, unsigned format_size
    );

private:

    // Implementation helpers
    void initialize_upload_texture(
        ID3D12Device2* device,
        size_t size
    );

    HRESULT suballocate_from_buffer(SIZE_T size, UINT align);

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_texture;
    D3D12_RESOURCE_STATES m_state;
    DXGI_FORMAT m_format;

    UINT8* m_data_begin = nullptr;
    UINT8* m_data_cur   = nullptr;
    UINT8* m_data_end   = nullptr;
};

}
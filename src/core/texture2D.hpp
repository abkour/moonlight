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

class Texture2D
{
public:

    Texture2D(
        ID3D12Device2* device, 
        ID3D12GraphicsCommandList* command_list, 
        DXGI_FORMAT format,
        void* data, unsigned width, unsigned height, unsigned format_size
    );

    ID3D12Resource* get_underlying();

private:

    void initialize_upload_buffer(ID3D12Device2* device, SIZE_T size);
    HRESULT suballocate_from_buffer(SIZE_T size, UINT align);

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
    UINT8* m_data_begin = nullptr;
    UINT8* m_data_cur   = nullptr;
    UINT8* m_data_end   = nullptr;
};

}
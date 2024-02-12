#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <dxcapi.h>

namespace moonlight
{

class RuntimeDXCCompiler
{
public:

    RuntimeDXCCompiler() = default;

    RuntimeDXCCompiler(
        const wchar_t* filename,
        const wchar_t* entry_point,
        const wchar_t* target,
        bool output_error_message = true
    );

    ID3DBlob* get_code_blob()
    {
        return m_code_blob.Get();
    }

    void recompile(bool output_error_message = true)
    {

    }

private:

    Microsoft::WRL::ComPtr<ID3DBlob> m_code_blob;
};

}
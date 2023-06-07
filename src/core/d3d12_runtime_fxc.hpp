#pragma once
#include <d3dcompiler.h>
#include <wrl.h>

namespace moonlight
{

class RuntimeFXCCompiler
{
public:

    RuntimeFXCCompiler() = default;
    RuntimeFXCCompiler(
        const wchar_t* filename,
        D3D_SHADER_MACRO* defines,
        ID3DInclude* include,
        const char* entry_point,
        const char* target,
        UINT flags_1,
        UINT flags_2,
        bool output_error_message = true
    );

    ID3DBlob* get_code_blob()
    {
        return m_code_blob.Get();
    }

    void recompile(bool output_error_message = true);

private:

    const wchar_t* m_filename;
    D3D_SHADER_MACRO* m_defines;
    ID3DInclude* m_include;
    const char* m_entry_point;
    const char* m_target;
    UINT m_flags_1;
    UINT m_flags_2;
    Microsoft::WRL::ComPtr<ID3DBlob> m_code_blob;
};

}
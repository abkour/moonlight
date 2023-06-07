#include "d3d12_runtime_fxc.hpp"
#include <cstdio>

namespace moonlight
{

RuntimeFXCCompiler::RuntimeFXCCompiler(
    const wchar_t* filename,
    D3D_SHADER_MACRO* defines,
    ID3DInclude* include,
    const char* entry_point,
    const char* target,
    UINT flags_1,
    UINT flags_2,
    bool output_error_message)
{
    m_filename = filename;
    m_defines = defines;
    m_include = include;
    m_entry_point = entry_point;
    m_target = target;
    m_flags_1 = flags_1;
    m_flags_2 = flags_2;

    Microsoft::WRL::ComPtr<ID3DBlob> error_blob;

    HRESULT hr = D3DCompileFromFile(
        m_filename,
        m_defines,
        m_include,
        m_entry_point,
        m_target,
        m_flags_1,
        m_flags_2,
        &m_code_blob,
        &error_blob
    );

    if (FAILED(hr) && output_error_message)
    {
        if (error_blob)
        {
            wchar_t error_msg[512];
            swprintf_s(error_msg, L"Compilation failed with errors:\n%hs\n",
                (const char*)error_blob->GetBufferPointer());
            OutputDebugStringW(error_msg);
        }
    }
}

void RuntimeFXCCompiler::recompile(bool output_error_message)
{
    Microsoft::WRL::ComPtr<ID3DBlob> error_blob;

    HRESULT hr = D3DCompileFromFile(
        m_filename,
        m_defines,
        m_include,
        m_entry_point,
        m_target,
        m_flags_1,
        m_flags_2,
        &m_code_blob,
        &error_blob
    );

    if (FAILED(hr) && output_error_message)
    {
        if (error_blob)
        {
            wchar_t error_msg[512];
            swprintf_s(error_msg, L"Compilation failed with errors:\n%hs\n",
                (const char*)error_blob->GetBufferPointer());
            OutputDebugStringW(error_msg);
        }
    }
}

}
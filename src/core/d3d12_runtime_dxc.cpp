#include "d3d12_runtime_dxc.hpp"
#include <cstdint>
#include <cstdio>

namespace moonlight
{

using namespace Microsoft::WRL;

RuntimeDXCCompiler::RuntimeDXCCompiler(
    const wchar_t* filename,
    const wchar_t* entry_point,
    const wchar_t* target,
    bool output_error_message)
{
    ComPtr<IDxcLibrary> library;
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed creating instance!");
    }

    // IDxcCompiler is deprecated. You should obtain an updated header dxcapi.h header,
    // that includes IDxcCompiler3.
    // See: https://learn.microsoft.com/en-us/windows/win32/api/dxcapi/ns-dxcapi-idxccompiler
    ComPtr<IDxcCompiler> compiler;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed creating instance2!");
    }

    uint32_t codePage = CP_UTF8;
    ComPtr<IDxcBlobEncoding> sourceBlob;

    hr = library->CreateBlobFromFile(filename, &codePage, &sourceBlob);
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed creating blob from file!");
    }

    ComPtr<IDxcOperationResult> result;
    hr = compiler->Compile(
        sourceBlob.Get(), // pSource
        filename, // pSourceName
        entry_point, // pEntryPoint
        target, // pTargetProfile
        NULL, 0, // pArguments, argCount
        NULL, 0, // pDefines, defineCount
        NULL, // pIncludeHandler
        &result); // ppResult
    if (SUCCEEDED(hr))
        result->GetStatus(&hr);
    if (FAILED(hr))
    {
        if (result)
        {
            ComPtr<IDxcBlobEncoding> error_blob;
            hr = result->GetErrorBuffer(&error_blob);
            if (SUCCEEDED(hr) && error_blob)
            {
                wchar_t error_msg[512];
                swprintf_s(error_msg, L"Compilation failed with errors:\n%hs\n",
                    (const char*)error_blob->GetBufferPointer());
                OutputDebugStringW(error_msg);

                return;
            }
        }
    }
    hr = result->GetResult((IDxcBlob**)m_code_blob.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringA("Failed creating result!");
    }
}

}
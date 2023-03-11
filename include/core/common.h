#pragma once

typedef unsigned int uint;
#include <Windows.h>
#include <wrl.h>
#include "dependency/d3dx12.h"
#include <dxgi.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>//D3D 编译相关的
#include "dx_exception_class.h"
#include <type_traits>
using namespace Microsoft::WRL;
using namespace DirectX;


namespace yang{
    inline wchar_t* ConvertCharArrayToLPCWSTR(const char* char_array)
    {
        auto w_string = new wchar_t[4096];
        MultiByteToWideChar(CP_ACP, 0, char_array, -1, w_string, 4096);
        return w_string;
    }


    template<typename T>
    requires(!std::is_lvalue_reference_v<T>)
    T* get_rvalue_ptr(T&& v) {
        return &v;
    }

    inline DirectX::XMFLOAT4X4 Identity4x4()
    {
        static DirectX::XMFLOAT4X4 I(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);

        return I;
    }

    inline UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        return (byteSize + 255) & ~255;
    }

    inline ComPtr<ID3DBlob> CompileShader(
            const std::wstring& fileName,
            const D3D_SHADER_MACRO* defines,
            const std::string& enteryPoint,
            const std::string& target)
    {
        //若处于调试模式，则使用调试标志
        UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
        //用调试模式来编译着色器 | 指示编译器跳过优化阶段
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

        HRESULT hr = S_OK;

        ComPtr<ID3DBlob> byteCode = nullptr;
        ComPtr<ID3DBlob> errors;
        hr = D3DCompileFromFile(fileName.c_str(), //hlsl源文件名
                                defines,	//高级选项，指定为空指针
                                D3D_COMPILE_STANDARD_FILE_INCLUDE,	//高级选项，可以指定为空指针
                                enteryPoint.c_str(),	//着色器的入口点函数名
                                target.c_str(),		//指定所用着色器类型和版本的字符串
                                compileFlags,	//指示对着色器断代码应当如何编译的标志
                                0,	//高级选项
                                &byteCode,	//编译好的字节码
                                &errors);	//错误信息

        if (errors != nullptr)
        {
            OutputDebugStringA((char*)errors->GetBufferPointer());
        }
        THROW_IF_FAILED(hr);

        return byteCode;
    }
}
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
#include <type_traits>
using namespace Microsoft::WRL;
using namespace DirectX;


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
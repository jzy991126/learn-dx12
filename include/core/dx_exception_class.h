#pragma once
#include <string>
#include <comdef.h>

namespace yang
{
	inline std::wstring AnsiToWString(const std::string& str)
	{
		WCHAR buffer[512];
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
		return { buffer };
	}

	class DxException
	{
	private:
		HRESULT error_code_ = S_OK;
		std::wstring function_name_, file_name_;
		int line_number_ = -1;
	public:
		DxException() = default;
		DxException(HRESULT result, std::wstring function_name, std::wstring file_name, int line_number);
		[[nodiscard]] std::wstring ToString()const;

	};

#ifndef ThrowIfFailed
#define THROW_IF_FAILED(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = yang::AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw yang::DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

}

#include "dx_exception_class.h"

namespace yang
{
	DxException::DxException(const HRESULT result, std::wstring function_name, std::wstring file_name,
	                         int line_number):error_code_(result),function_name_(std::move(function_name)),file_name_(
		                                          std::move(file_name)),line_number_(line_number)
	{
	}

	std::wstring DxException::ToString() const
	{
		const _com_error error(error_code_);
		const auto message = error.ErrorMessage();
		return function_name_+L"failed in "+file_name_+L";line "+ std::to_wstring(line_number_) + L"; error: " + message;
	}
}

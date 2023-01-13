#pragma once

typedef unsigned int uint;


inline wchar_t* ConvertCharArrayToLPCWSTR(const char* char_array)
{
	auto w_string = new wchar_t[4096];
    MultiByteToWideChar(CP_ACP, 0, char_array, -1, w_string, 4096);
    return w_string;
}
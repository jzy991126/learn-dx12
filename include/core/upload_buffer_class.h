#pragma once
#include "common.h"

namespace yang
{
	template<typename T>
	class UploadBufferResource
	{
		UploadBufferResource(ComPtr<ID3D12Device> d3dDevice, UINT elementCount, bool isConstantBuffer);
	};

	
}

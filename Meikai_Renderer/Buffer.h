#pragma once
#include "Resource.h"
class DXApp;
class Buffer : public Resource
{
public:
	explicit Buffer(DXApp* appPtr, const std::wstring& name = L"");
	explicit Buffer(DXApp* appPtr, const D3D12_RESOURCE_DESC& resDesc,
		size_t numElements, size_t elementSize,
		const std::wstring& name = L"");;
};


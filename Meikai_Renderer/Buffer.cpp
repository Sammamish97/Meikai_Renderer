#include "Buffer.h"
#include "DXApp.h"

Buffer::Buffer(DXApp* appPtr, const std::wstring& name)
	:Resource(appPtr, name)
{
}

Buffer::Buffer(DXApp* appPtr, const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elementSize,
	const std::wstring& name)
		:Resource(appPtr, nullptr, name)
{
}

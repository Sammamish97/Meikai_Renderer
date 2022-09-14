#include "IndexBuffer.h"
#include <cassert>
#include "DXApp.h"

IndexBuffer::IndexBuffer(DXApp* appPtr, const std::wstring& name)
	:Buffer(appPtr, name), mNumIndicies(0), mIndexFormat(DXGI_FORMAT_UNKNOWN), mIndexBufferView({})
{
}

IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
    assert(elementSize == 2 || elementSize == 4 && "Indices must be 16, or 32-bit integers.");

    mNumIndicies = numElements;
    mIndexFormat = (elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    mIndexBufferView.BufferLocation = mResource->GetGPUVirtualAddress();
    mIndexBufferView.SizeInBytes = static_cast<UINT>(numElements * elementSize);
    mIndexBufferView.Format = mIndexFormat;
}

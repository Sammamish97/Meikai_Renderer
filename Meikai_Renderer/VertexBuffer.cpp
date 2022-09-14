#include "VertexBuffer.h"
#include "DXApp.h"
VertexBuffer::VertexBuffer(DXApp* appPtr, const std::wstring& name)
	:Buffer(appPtr, name), mNumVertices(0), mVertexStride(0), mVertexBufferView({}) 
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::CreateVertexBufferView(size_t numElements, size_t elementSize)
{
    mNumVertices = numElements;
    mVertexStride = elementSize;

    mVertexBufferView.BufferLocation = mResource->GetGPUVirtualAddress();
    mVertexBufferView.SizeInBytes = static_cast<UINT>(mNumVertices * mVertexStride);
    mVertexBufferView.StrideInBytes = static_cast<UINT>(mVertexStride);
}

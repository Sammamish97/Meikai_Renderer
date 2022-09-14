#pragma once
#include "Buffer.h"
class DXApp;
class VertexBuffer : public Buffer
{
public:
	VertexBuffer(DXApp* appPtr, const std::wstring& name = L"");
	virtual ~VertexBuffer();

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
    {
        return mVertexBufferView;
    }

    size_t GetNumVertices() const
    {
        return mNumVertices;
    }

    size_t GetVertexStride() const
    {
        return mVertexStride;
    }

    void CreateVertexBufferView(size_t numElements, size_t elementSize);

private:
    size_t mNumVertices;
    size_t mVertexStride;

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
};


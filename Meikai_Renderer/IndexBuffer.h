#pragma once
#include "Buffer.h"

class DXApp;
class IndexBuffer : public Buffer
{
public:
	IndexBuffer(DXApp* appPtr, const std::wstring& name = L"");
	virtual ~IndexBuffer();
	void CreateViews(size_t numElements, size_t elementSize);

    size_t GetNumIndicies() const
    {
        return mNumIndicies;
    }

    DXGI_FORMAT GetIndexFormat() const
    {
        return mIndexFormat;
    }

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
    {
        return mIndexBufferView;
    }

private:
    size_t mNumIndicies;
    DXGI_FORMAT mIndexFormat;

    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
};


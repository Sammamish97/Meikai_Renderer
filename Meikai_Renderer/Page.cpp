#include "Page.h"
#include "DXApp.h"
#include "DXUtil.h"
#include <d3dx12.h>

Page::Page(DXApp* appPtr, size_t sizeInBytes)
	:mApp(appPtr),
	mPageSize(sizeInBytes),
	mOffset(0),
	mCPUPtr(nullptr),
	mGPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
}

UploadPage::UploadPage(DXApp* appPtr, size_t sizeInBytes)
	:Page(appPtr, sizeInBytes)
{
	auto device = appPtr->GetDevice();

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mPageSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mResource)
	))

	mGPUPtr = mResource->GetGPUVirtualAddress();
	mResource->Map(0, nullptr, &mCPUPtr);
}

Page::~Page()
{
	mResource->Unmap(0, nullptr);
	mCPUPtr = nullptr;
	mGPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = AlignUp(mOffset, alignment);

	return alignedOffset + alignedSize <= mPageSize;
}

void Page::Reset()
{
	mOffset = 0;
}

UploadAllocation UploadPage::Allocate(const void* data, size_t sizeInBytes, size_t alignment)
{
	if (HasSpace(sizeInBytes, alignment) == false)
	{
		throw std::bad_alloc();
	}

	size_t alignedSize = AlignUp(sizeInBytes, alignment);
	mOffset = AlignUp(mOffset, alignment);

	UploadAllocation allocation;
	allocation.Size = alignedSize;
	allocation.CPU = static_cast<uint8_t*>(mCPUPtr) + mOffset;
	allocation.GPU = mGPUPtr + mOffset;

	if (data != nullptr)
	{
		allocation.Copy(data, alignedSize);
	}
	mOffset += alignedSize;
	return allocation;
}


void Allocation::Reset()
{
	CPU = nullptr;
	GPU = D3D12_GPU_VIRTUAL_ADDRESS(0);;
	Size = 0;
}

void UploadAllocation::Copy(const void* data, size_t size)
{
	if (size > Size)
	{
		throw std::bad_alloc();
	}
	memcpy(CPU, data, size);
}

#include "Page.h"
#include "DXApp.h"
#include "DXUtil.h"

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

DefaultPage::DefaultPage(DXApp* appPtr, size_t sizeInBytes)
	:Page(appPtr, sizeInBytes)
{
	auto device = appPtr->GetDevice();

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mPageSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mResource)
	))

	mGPUPtr = mResource->GetGPUVirtualAddress();
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

UploadAllocation UploadPage::Allocate(void* data, size_t sizeInBytes, size_t alignment)
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
	allocation.Copy(data, alignedSize);

	mOffset += alignedSize;

	return allocation;
}

DefaultAllocation DefaultPage::Allocate(void* data, size_t sizeInBytes, size_t alignment
	, void* stagingCPU, D3D12_GPU_VIRTUAL_ADDRESS stagingGPU)
{
	if (HasSpace(sizeInBytes, alignment) == false)
	{
		throw std::bad_alloc();
	}
	size_t alignedSize = AlignUp(sizeInBytes, alignment);
	mOffset = AlignUp(mOffset, alignment);

	memcpy(stagingCPU, data, sizeInBytes);
	ComPtr<ID3D12GraphicsCommandList2> tempList;
	mApp->mCommandMgr->AllocateTempList(tempList);
	
	//D3D12_SUBRESOURCE_DATA subresourceData = {};
	//subresourceData.pData = bufferData;
	//subresourceData.RowPitch = bufferSize;
	//subresourceData.SlicePitch = subresourceData.RowPitch;

	//TODO: Start from here
	////This is command function. Therefore, need to execute command list & flush queue.
	//UpdateSubresources(commandList.Get(),
	//	*pDestinationResource, *pIntermediateResource,
	//	0, 0, 1, &subresourceData);
	

	DefaultAllocation allocation;
	allocation.Size = alignedSize;
	allocation.CPU = nullptr;
	allocation.GPU = mGPUPtr + mOffset;
	allocation.Copy(data, alignedSize, stagingCPU, stagingGPU);


	mOffset += alignedSize;

	return allocation;
}

void Allocation::Reset()
{
	CPU = nullptr;
	GPU = D3D12_GPU_VIRTUAL_ADDRESS(0);;
	Size = 0;
}

void UploadAllocation::Copy(void* data, size_t size)
{
	if (size > Size)
	{
		throw std::bad_alloc();
	}
	memcpy(CPU, data, size);
}

void DefaultAllocation::Copy(void* data, size_t size, void* stagingCPU, D3D12_GPU_VIRTUAL_ADDRESS stagingGPU)
{
	//Static 선언되어있는 Staging용 upload버퍼에 접근해서 upload 버퍼에 복사, 그리고 upload buffer의 내용을 default로 복사한다.
	if (size > Size)
	{
		throw std::bad_alloc();
	}
	memcpy(stagingCPU, data, size);
	ComPtr<ID3D12CommandList> copyCmdList;

	//
	// CommandList를 일시적으로 생성, Command list를 통해
	// Upload에 있는 data를 Default로 복사 해야한다.
	// 
	//
}

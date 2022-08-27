#include "ResourceAllocator.h"
#include "DXApp.h"
#include "DXUtil.h"

ResourceAllocator::ResourceAllocator(DXApp* appPtr, size_t pageSize)
	:mApp(appPtr), mPageSize(pageSize)
{
	InitStagingBuffer();
}

ResourceAllocator::~ResourceAllocator()
{
	
}

void ResourceAllocator::InitStagingBuffer()
{
	auto device = mApp->GetDevice();
	
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mPageSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mStagingResource)
	))

	mStagingGPU = mStagingResource->GetGPUVirtualAddress();
	mStagingResource->Map(0, nullptr, &mStagingCPU);
}

UploadAllocation ResourceAllocator::AllocateToUploadHeap(void* data, size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > mPageSize)
	{
		//Upload heap don't need huge area.
		throw std::bad_alloc();
	}

	if (mCurrentUploadPage == nullptr || mCurrentUploadPage->HasSpace(sizeInBytes, alignment))
	{
		mCurrentUploadPage = RequestUploadPage();
	}
	mCurrentUploadPage->Allocate(data, sizeInBytes, alignment);
	//TODO: 여기서 page에게 data를 넘기고, Page안에서 GPU로의 data복사가 일어나게 해야 한다.
}

DefaultAllocation ResourceAllocator::AllocateToDefaultHeap(void* data, size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > mPageSize)
	{
		//For IBL or other huge image file.
		//
	}
	if (mCurrentDefaultPage == nullptr || mCurrentDefaultPage->HasSpace(sizeInBytes, alignment))
	{
		mCurrentDefaultPage = RequestDefaultPage();
	}
	return mCurrentDefaultPage->Allocate(data, sizeInBytes, alignment, mStagingCPU, mStagingResource);
}

std::shared_ptr<UploadPage> ResourceAllocator::RequestUploadPage()
{
	std::shared_ptr<UploadPage> newPage;
	if (mUploadAvailablePages.empty() == false)
	{
		newPage = mUploadAvailablePages.front();
		mUploadAvailablePages.pop_front();
	}
	else
	{
		newPage = std::make_shared<UploadPage>(mApp, mPageSize);
		mUploadPagePool.push_back(newPage);
	}
	return newPage;
}

std::shared_ptr<DefaultPage> ResourceAllocator::RequestDefaultPage()
{
	std::shared_ptr<DefaultPage> newPage;
	if (mDefaultAvailablePages.empty() == false)
	{
		newPage = mDefaultAvailablePages.front();
		mDefaultAvailablePages.pop_front();
	}
	else
	{
		newPage = std::make_shared<DefaultPage>(mApp, mPageSize);
		mDefaultPagePool.push_back(newPage);
	}
	return newPage;
}

void ResourceAllocator::Reset()
{

}
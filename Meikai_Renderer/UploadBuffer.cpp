#include "UploadBuffer.h"
#include "DXApp.h"
#include "DXUtil.h"

UploadBuffer::UploadBuffer(DXApp* appPtr, size_t pageSize)
	:mApp(appPtr), mPageSize(pageSize)
{
}

UploadBuffer::~UploadBuffer()
{
}


UploadAllocation UploadBuffer::AllocateToUploadHeap(const void* data, size_t sizeInBytes, size_t alignment)
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
	return mCurrentUploadPage->Allocate(data, sizeInBytes, alignment);
}

std::shared_ptr<UploadPage> UploadBuffer::RequestUploadPage()
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

void UploadBuffer::Reset()
{
	mCurrentUploadPage = nullptr;
	mUploadAvailablePages = mUploadPagePool;
	for(auto page : mUploadAvailablePages)
	{
		page->Reset();
	}
}
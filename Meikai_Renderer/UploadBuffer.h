#pragma once

#include "MemDefine.h"
#include "Page.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

using namespace Microsoft::WRL;
class DXApp;
class UploadBuffer
{
    DXApp* mApp;

public:
    explicit UploadBuffer(DXApp* appPtr, size_t pageSize = _2MB);
    virtual ~UploadBuffer();
    size_t GetPageSize() const { return mPageSize; }

    UploadAllocation AllocateToUploadHeap(const void* data, size_t sizeInBytes, size_t alignment);
    void Reset();

private:

    using UploadPagePool = std::deque<std::shared_ptr<UploadPage>>;

    std::shared_ptr<UploadPage> RequestUploadPage();

    UploadPagePool mUploadPagePool;
    UploadPagePool mUploadAvailablePages;

    std::shared_ptr<UploadPage> mCurrentUploadPage;

    size_t mPageSize;
};
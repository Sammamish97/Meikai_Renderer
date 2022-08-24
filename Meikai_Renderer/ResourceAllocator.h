#pragma once

#include "MemDefine.h"
#include "Page.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

using namespace Microsoft::WRL;
class DXApp;
class ResourceAllocator
{
    DXApp* mApp;

public:
    explicit ResourceAllocator(DXApp* appPtr, size_t pageSize = _2MB);
    void InitStagingBuffer();

    virtual ~ResourceAllocator();
    size_t GetPageSize() const { return mPageSize; }

    UploadAllocation AllocateToUploadHeap(void* data, size_t sizeInBytes, size_t alignment);
    DefaultAllocation AllocateToDefaultHeap(void* data, size_t sizeInBytes, size_t alignment);

    void Reset();

private:

    using UploadPagePool = std::deque<std::shared_ptr<UploadPage>>;
    using DefaultPagePool = std::deque<std::shared_ptr<DefaultPage>>;

    std::shared_ptr<UploadPage> RequestUploadPage();
    std::shared_ptr<DefaultPage> RequestDefaultPage();

    UploadPagePool mUploadPagePool;
    UploadPagePool mUploadAvailablePages;

    DefaultPagePool mDefaultPagePool;
    DefaultPagePool mDefaultAvailablePages;

    std::shared_ptr<UploadPage> mCurrentUploadPage;
    std::shared_ptr<DefaultPage> mCurrentDefaultPage;

    ComPtr<ID3D12Resource> mStagingResource;
    void* mStagingCPU;
    D3D12_GPU_VIRTUAL_ADDRESS mStagingGPU;

    size_t mPageSize;
};
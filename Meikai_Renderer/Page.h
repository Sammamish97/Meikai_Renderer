#pragma once
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;
class DXApp;

struct Allocation
{
    void Reset();

    void* CPU;
    D3D12_GPU_VIRTUAL_ADDRESS GPU;
    size_t Size;
};

struct UploadAllocation : public Allocation
{
    void Copy(void* data, size_t size);
};

struct DefaultAllocation : public Allocation
{
    void Copy(void* data, size_t size, void* stagingCPU, D3D12_GPU_VIRTUAL_ADDRESS stagingGPU);
};

struct Page
{
protected:
    DXApp* mApp;

public:
    Page(DXApp* appPtr, size_t sizeInBytes);
    ~Page();

    // Check to see if the page has room to satisfy the requested
    // allocation.
    bool HasSpace(size_t sizeInBytes, size_t alignment) const;

    // Reset the page for reuse.
    void Reset();

protected:

    ComPtr<ID3D12Resource> mResource;

    // Base pointer.
    void* mCPUPtr;
    D3D12_GPU_VIRTUAL_ADDRESS mGPUPtr;

    // Allocated page size.
    size_t mPageSize;
    // Current allocation offset in bytes.
    size_t mOffset;
};

struct UploadPage : public Page
{
    UploadPage(DXApp* appPtr, size_t sizeInBytes)
        : Page(appPtr, sizeInBytes){}
    UploadAllocation Allocate(void* data, size_t sizeInBytes, size_t alignment);
};

struct DefaultPage : public Page
{
    DefaultPage(DXApp* appPtr, size_t sizeInBytes)
        : Page(appPtr, sizeInBytes) {}
    DefaultAllocation Allocate(void* data, size_t sizeInBytes, size_t alignment, void* stagingCPU, D3D12_GPU_VIRTUAL_ADDRESS stagingGPU);
};


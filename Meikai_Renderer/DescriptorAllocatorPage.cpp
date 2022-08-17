#include "DescriptorAllocatorPage.h"
#include "DXApp.h"
#include "DXUtil.h"
DescriptorAllocatorPage::DescriptorAllocatorPage(DXApp* appPtr, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
	:mHeapType(type), mNumDescriptorsInHeap(numDescriptors)
{
	auto device = appPtr->GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = mHeapType;
	heapDesc.NumDescriptors = mNumDescriptorsInHeap;

	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDescriptorHeap)))

	mBaseDescriptor = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(mHeapType);
	mNumFreeHandles = mNumDescriptorsInHeap;

	AddNewBlock(0, mNumFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
	return mHeapType;
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
	return mNumFreeHandles;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
	return mFreeListBySize.lower_bound(numDescriptors) != mFreeListBySize.end();
}

void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
	auto offsetIt = mFreeListByOffset.emplace(offset, numDescriptors);
	auto sizeIt = mFreeListBySize.emplace(numDescriptors, offsetIt.first);
	offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{

}

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{

}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber)
{

}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{

}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{

}
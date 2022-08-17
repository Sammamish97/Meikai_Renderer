#include "DescriptorAllocatorPage.h"
#include "DXApp.h"
#include "DXUtil.h"
DescriptorAllocatorPage::DescriptorAllocatorPage(DXApp* appPtr, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
	:mApp(appPtr), mHeapType(type), mNumDescriptorsInHeap(numDescriptors)
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

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	if (numDescriptors > mNumFreeHandles)
	{
		return DescriptorAllocation(mApp);
	}

	auto smallestBlockIt = mFreeListBySize.lower_bound(numDescriptors);
	if (smallestBlockIt == mFreeListBySize.end())
	{
		return DescriptorAllocation(mApp);
	}

	auto blockSize = smallestBlockIt->first;
	auto offsetIt = smallestBlockIt->second;
	auto offset = offsetIt->first;

	mFreeListBySize.erase(smallestBlockIt);
	mFreeListByOffset.erase(offsetIt);

	// Compute the new free block that results from splitting this block.
	auto newOffset = offset + numDescriptors;
	auto newSize = blockSize - numDescriptors;

	if (newSize > 0)
	{
		// If the allocation didn't exactly match the requested size,
		// return the left-over to the free list.
		AddNewBlock(newOffset, newSize);
	}

	// Decrement free handles.
	mNumFreeHandles -= numDescriptors;

	return DescriptorAllocation(mApp,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mBaseDescriptor, offset, mDescriptorHandleIncrementSize),
		numDescriptors, mDescriptorHandleIncrementSize, shared_from_this());
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	return static_cast<uint32_t>(handle.ptr - mBaseDescriptor.ptr) / mDescriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor, uint64_t frameNumber)
{
	// Compute the offset of the descriptor within the descriptor heap.
	auto offset = ComputeOffset(descriptor.GetDescriptorHandle());

	std::lock_guard<std::mutex> lock(mAllocationMutex);

	// Don't add the block directly to the free list until the frame has completed.
	mStaleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(mAllocationMutex);

	while (!mStaleDescriptors.empty() && mStaleDescriptors.front().FrameNumber <= frameNumber)
	{
		auto& staleDescriptor = mStaleDescriptors.front();

		// The offset of the descriptor in the heap.
		auto offset = staleDescriptor.Offset;
		// The number of descriptors that were allocated.
		auto numDescriptors = staleDescriptor.Size;

		FreeBlock(offset, numDescriptors);

		mStaleDescriptors.pop();
	}
}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
	auto nextBlockIt = mFreeListByOffset.upper_bound(offset);
	auto prevBlockIt = nextBlockIt;

	if (prevBlockIt != mFreeListByOffset.begin())
	{
		--prevBlockIt;
	}
	else
	{
		prevBlockIt = mFreeListByOffset.end();
	}

	mNumFreeHandles += numDescriptors;

	if (prevBlockIt != mFreeListByOffset.end() &&
		offset == prevBlockIt->first + prevBlockIt->second.Size)
	{
		offset = prevBlockIt->first;
		numDescriptors += prevBlockIt->second.Size;

		// Remove the previous block from the free list.
		mFreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
		mFreeListByOffset.erase(prevBlockIt);
	}

	if (nextBlockIt != mFreeListByOffset.end() &&
		offset + numDescriptors == nextBlockIt->first)
	{
		numDescriptors += nextBlockIt->second.Size;

		mFreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
		mFreeListByOffset.erase(nextBlockIt);
	}
	AddNewBlock(offset, numDescriptors);
}
#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include <map>	
#include <memory>
#include <mutex>	
#include <queue>

#include "DescriptorAllocation.h"
using namespace Microsoft::WRL;
class DXApp;
class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
	DXApp* mApp;
public:
	DescriptorAllocatorPage(DXApp* appPtr, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;
	bool HasSpace(uint32_t numDescriptors) const;
	uint32_t NumFreeHandles() const;
	DescriptorAllocation Allocate(uint32_t numDescriptors);
	void Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber);
	void ReleaseStaleDescriptors(uint64_t frameNumber);

protected:
	uint32_t ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void AddNewBlock(uint32_t offset, uint32_t numDescriptors);
	void FreeBlock(uint32_t offset, uint32_t numDescriptors);

private:
	using OffsetType = uint32_t;
	using SizeType = uint32_t;

	struct FreeBlockInfo;

	using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;
	using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

	struct FreeBlockInfo
	{
		FreeBlockInfo(SizeType size)
			:Size(size){}

		SizeType Size;
		FreeListBySize::iterator FreeListBySizeIt;
	};

	struct StaleDescriptorInfo
	{
		StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame)
			:Offset(offset), Size(size), FrameNumber(frame){}

		OffsetType Offset;
		SizeType Size;
		uint64_t FrameNumber;
	};

	using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

	FreeListByOffset mFreeListByOffset;
	FreeListBySize mFreeListBySize;
	StaleDescriptorQueue mStaleDescriptors;

	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBaseDescriptor;
	uint32_t mDescriptorHandleIncrementSize;
	uint32_t mNumDescriptorsInHeap;
	uint32_t mNumFreeHandles;

	std::mutex mAllocationMutex;
};


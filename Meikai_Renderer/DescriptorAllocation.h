#pragma once
#include <d3d12.h>
#include <cstdint>
#include <memory>
class DescriptorAllocatorPage;
class DXApp;
class DescriptorAllocation
{
	DXApp* mApp = nullptr;
public:
	DescriptorAllocation(DXApp* appPtr);
	DescriptorAllocation(DXApp* appPtr, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page);

	~DescriptorAllocation();

	// Copies are not allowed.
	DescriptorAllocation(const DescriptorAllocation&) = delete;
	DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

	// Move is allowed.
	DescriptorAllocation(DescriptorAllocation&& allocation);
	DescriptorAllocation& operator=(DescriptorAllocation&& other);

	bool IsNull() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

	uint32_t GetNumHandles() const;

	std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;

private:
	void Free();

	D3D12_CPU_DESCRIPTOR_HANDLE mDescriptor;
	uint32_t mNumHandles;
	uint32_t mDescriptorSize;

	std::shared_ptr<DescriptorAllocatorPage> mPage;
};


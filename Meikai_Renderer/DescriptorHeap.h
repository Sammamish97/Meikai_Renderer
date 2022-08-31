#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include <map>
using namespace Microsoft::WRL;
class DXApp;
class DescriptorHeap
{
	DXApp* mApp;
public:
	DescriptorHeap(DXApp* appPtr, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorSize, UINT maxDescriptors);
	UINT GetNextAvailableIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT idx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT idx) const;
	ComPtr<ID3D12DescriptorHeap>& GetDescriptorHeap();
private:
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;

	UINT mDescriptorSize;
	UINT mMaxDescriptor;

	UINT mOffset;
};


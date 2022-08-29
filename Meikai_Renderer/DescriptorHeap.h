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
	D3D12_CPU_DESCRIPTOR_HANDLE GetNextHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE operator[](UINT idx);

private:
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;
	UINT mDescriptorSize;
	UINT mMaxDescriptor;

	UINT mOffset;
};


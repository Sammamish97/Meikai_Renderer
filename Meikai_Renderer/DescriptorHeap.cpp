#include "DescriptorHeap.h"
#include "DXApp.h"
DescriptorHeap::DescriptorHeap(DXApp* appPtr, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorSize, UINT maxDescriptors)
	:mApp(appPtr), mHeapType(heapType), mDescriptorSize(descriptorSize), mOffset(0), mMaxDescriptor(maxDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc;
	HeapDesc.NumDescriptors = mMaxDescriptor;
	HeapDesc.Type = mHeapType;
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HeapDesc.NodeMask = 0;
	mApp->GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(mDescriptorHeap.GetAddressOf()));
}

//Allocate and return offset of descriptor.
UINT DescriptorHeap::GetNextAvailableIndex()
{
	return mOffset++;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(UINT idx) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (INT)idx, mDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT idx) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), (INT)idx, mDescriptorSize);
}

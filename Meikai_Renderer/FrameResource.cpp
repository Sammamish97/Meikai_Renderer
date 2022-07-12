#include "FrameResource.h"
#include "DXUtil.h"
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
	D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mCmdListAlloc.GetAddressOf())))

	mPassCB = std::make_unique<UploadBuffer<PassCB>>(device, passCount, true);
	mObjectCB = std::make_unique<UploadBuffer<ObjectCB>>(device, objectCount, true);
	mGeometryCB = std::make_unique<UploadBuffer<GeometryCB>>(device, 1, true);
}

FrameResource::~FrameResource()
{
}

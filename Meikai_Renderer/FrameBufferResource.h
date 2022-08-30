#pragma once
#include <wrl.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;

struct FrameBufferResource
{
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	ComPtr<ID3D12Resource> mPositionMap;
	ComPtr<ID3D12Resource> mNormalMap;
	ComPtr<ID3D12Resource> mAlbedoMap;
	ComPtr<ID3D12Resource> mMetalicMap;
	ComPtr<ID3D12Resource> mRoughnessMap;
};

struct FrameBufferDescriptorIndex
{
	UINT mPositionDescRtvIdx;
	UINT mPositionDescSrvIdx;

	UINT mNormalDescRtvIdx;
	UINT mNormalDescSrvIdx;

	UINT mAlbedoDescRtvIdx;
	UINT mAlbedoDescSrvIdx;

	UINT mRoughnessDescRtvIdx;
	UINT mRoughnessDescSrvIdx;

	UINT mMetalicDescRtvIdx;
	UINT mMetalicDescSrvIdx;

	UINT mDepthStencilSrvIdx;

	UINT mDepthStencilDsvIdx;
};
#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include <memory>
#include "Texture.h"

using namespace Microsoft::WRL;

struct FrameBufferResource
{
	std::shared_ptr<Texture> mRenderTarget;
	std::shared_ptr<Texture> mDepthStencilBuffer;
	std::shared_ptr<Texture> mPositionMap;
	std::shared_ptr<Texture> mNormalMap;
	std::shared_ptr<Texture> mAlbedoMap;
	std::shared_ptr<Texture> mMetalicMap;
	std::shared_ptr<Texture> mRoughnessMap;
	std::shared_ptr<Texture> mSsaoMap;
};

struct IBLResource
{
	std::shared_ptr<Texture> mHDRImage;
	std::shared_ptr<Texture> mCubeMap;
	std::shared_ptr<Texture> mSkyboxCubeMap;
	std::shared_ptr<Texture> mDiffuseCubeMap;
	std::shared_ptr<Texture> mSpecularCubeMap;
};

struct FrameBufferDescriptorIndex
{
	UINT mRenderTargetRtvIdx;
	UINT mRenderTargetSrvIdx;

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

	UINT mSsaoDescRtvIdx;
	UINT mSsaoDescSrvIdx;

	UINT mDepthStencilSrvIdx;
	UINT mDepthStencilDsvIdx;
};

struct IBLDescriptorIndex
{
	UINT mHDRImageSrvIndex;

	UINT mCubemapSrvIndex;
	UINT mCubemapUavIndex;

	UINT mSkyboxCubemapSrvIndex;
	UINT mSkyboxCubemapUavIndex;

	UINT mDiffuseCubemapSrvIndex;
	UINT mDiffuseCubemapUavIndex;
};
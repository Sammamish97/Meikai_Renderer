#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include "ResourceAllocator.h"
//TODO: Resource들을 allocator를 통해 생성되도록 바꾸기
using namespace Microsoft::WRL;

struct MaterialResource
{
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	ComPtr<ID3D12Resource> mPositionMap;
	ComPtr<ID3D12Resource> mNormalMap;
	ComPtr<ID3D12Resource> mAlbedoMap;
	ComPtr<ID3D12Resource> mMetalicMap;
	ComPtr<ID3D12Resource> mRoughnessMap;
};
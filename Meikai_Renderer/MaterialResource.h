#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include "ResourceAllocator.h"
//TODO: Resource들을 allocator를 통해 생성되도록 바꾸기
using namespace Microsoft::WRL;

struct MaterialResource
{
	DefaultAllocation mDepthStencilBuffer;
	DefaultAllocation mPositionMap;
	DefaultAllocation mNormalMap;
	DefaultAllocation mAlbedoMap;
	DefaultAllocation mMetalicMap;
	DefaultAllocation mRoughnessMap;
};
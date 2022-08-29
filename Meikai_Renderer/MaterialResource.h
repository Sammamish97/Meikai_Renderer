#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include "ResourceAllocator.h"
//TODO: Resource���� allocator�� ���� �����ǵ��� �ٲٱ�
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
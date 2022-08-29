#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include "ResourceAllocator.h"
//TODO: Resource���� allocator�� ���� �����ǵ��� �ٲٱ�
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
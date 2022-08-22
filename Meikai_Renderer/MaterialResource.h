#pragma once
#include <wrl.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;

struct MaterialResource
{
	ComPtr<ID3D12Resource> mPositionMap;
	ComPtr<ID3D12Resource> mNormalMap;
	ComPtr<ID3D12Resource> mAlbedoMap;
	ComPtr<ID3D12Resource> mMetalicMap;
	ComPtr<ID3D12Resource> mRoughnessMap;
};
#pragma once
#include <wrl.h>
#include <d3dx12.h>
#include <string>
#include <optional>
#include "Resource.h"


using Microsoft::WRL::ComPtr;
class DXApp;

class Texture : public Resource
{
public:
	explicit Texture(DXApp* mApp,
		const std::wstring& name = L"");

	explicit Texture(DXApp* mApp, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_CLEAR_VALUE* clearValue = nullptr,
		
		D3D12_SRV_DIMENSION srvDim = D3D12_SRV_DIMENSION_UNKNOWN, D3D12_UAV_DIMENSION uavDim = D3D12_UAV_DIMENSION_UNKNOWN,
		const std::wstring& name = L"");

	explicit Texture(DXApp* mApp, ComPtr<ID3D12Resource> resource,
		
		D3D12_SRV_DIMENSION srvDim = D3D12_SRV_DIMENSION_UNKNOWN, D3D12_UAV_DIMENSION uavDim = D3D12_UAV_DIMENSION_UNKNOWN,
		const std::wstring& name = L"");

	Texture(const Texture& copy);
	Texture(Texture&& copy);

	Texture& operator=(const Texture& other);
	Texture& operator=(Texture&& other);

	D3D12_UNORDERED_ACCESS_VIEW_DESC GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, UINT mipSlice, UINT arraySlice = 0, UINT planeSlice = 0);

	virtual ~Texture();

	void CreateViews(D3D12_SRV_DIMENSION srvDim, D3D12_UAV_DIMENSION uavDim);

	void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

	static DXGI_FORMAT GetUAVCompatableFormat(DXGI_FORMAT format);
	static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);


public:
	//Later, maybe same type of descriptor can be multiple.
	std::optional<UINT> mRTVDescIDX;
	std::optional<UINT> mSRVDescIDX;
	std::optional<UINT> mDSVDescIDX;
	std::optional<UINT> mUAVDescIDX;
};


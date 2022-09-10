#pragma once
#include "TextureUsage.h"
#include <wrl.h>
#include <d3dx12.h>
#include <string>
#include <optional>
#include "Resource.h"


using Microsoft::WRL::ComPtr;
class DXApp;

struct Texture : public Resource
{
public:
	explicit Texture(DXApp* mApp, TextureUsage textureUsage = TextureUsage::Albedo,
		const std::wstring& name = L"");

	explicit Texture(DXApp* mApp, const D3D12_RESOURCE_DESC& resourceDesc,
		const D3D12_CLEAR_VALUE* clearValue = nullptr,
		TextureUsage textureUsage = TextureUsage::Albedo,
		const std::wstring& name = L"");

	explicit Texture(DXApp* mApp, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		TextureUsage textureUsage = TextureUsage::Albedo,
		const std::wstring& name = L"");

	Texture(const Texture& copy);
	Texture(Texture&& copy);

	Texture& operator=(const Texture& other);
	Texture& operator=(Texture&& other);

	virtual ~Texture();

	TextureUsage GetTextureUsage() const;
	void SetTextureUsage(TextureUsage textureUsage);

	void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

	void AllocateRTVDesc(UINT RTVdescIndex);
	void AllocateSRVDesc(UINT SRVdescIndex);
	void AllocateDSVDesc(UINT DSVdescIndex);
	void AllocateUAVDesc(UINT UAVDescIndex);

private:
	std::optional<UINT> mRTVDescIDX;
	std::optional<UINT> mSRVDescIDX;
	std::optional<UINT> mDSVDescIDX;
	std::optional<UINT> mUAVDescIDX;
	
	TextureUsage mTextureUsage;
};


#include "Texture.h"
#include "DXApp.h"
#include "DXUtil.h"

Texture::Texture(DXApp* mApp, TextureUsage textureUsage,
	const std::wstring& name)
	:Resource(mApp, name), mTextureUsage(textureUsage)
{
}

Texture::Texture(DXApp* mApp, const D3D12_RESOURCE_DESC& resourceDesc,
	const D3D12_CLEAR_VALUE* clearValue,
	TextureUsage textureUsage,
	const std::wstring& name)
	:Resource(mApp, resourceDesc, clearValue, name), mTextureUsage(textureUsage)
{
}

Texture::Texture(DXApp* mApp, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	TextureUsage textureUsage,
	const std::wstring& name)
	:Resource(mApp, resource, name), mTextureUsage(textureUsage)
{
}

Texture::Texture(const Texture& copy)
	:Resource(copy)
{

}

Texture::Texture(Texture&& copy)
	:Resource(copy)
{

}

Texture& Texture::operator=(const Texture& other)
{
	Resource::operator=(other);
	return *this;
}

Texture& Texture::operator=(Texture&& other)
{
	Resource::operator=(other);
	return *this;
}

Texture::~Texture()
{
}

void Texture::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize)
{
	if (mResource)
	{

	}
}


TextureUsage Texture::GetTextureUsage() const
{
	return mTextureUsage;
}


void Texture::SetTextureUsage(TextureUsage textureUsage)
{
	mTextureUsage = textureUsage;
}

void Texture::AllocateRTVDesc(UINT RTVdescIndex)
{
	mRTVDescIDX = RTVdescIndex;
}

void Texture::AllocateSRVDesc(UINT SRVdescIndex)
{
	mSRVDescIDX = SRVdescIndex;
}

void Texture::AllocateDSVDesc(UINT DSVdescIndex)
{
	mDSVDescIDX = DSVdescIndex;
}

void Texture::AllocateUAVDesc(UINT UAVDescIndex)
{
	mUAVDescIDX = UAVDescIndex;
}
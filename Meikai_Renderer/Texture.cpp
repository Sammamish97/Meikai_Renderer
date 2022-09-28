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

D3D12_UNORDERED_ACCESS_VIEW_DESC Texture::GetUAVDesc(const D3D12_RESOURCE_DESC& resDesc, UINT mipSlice, UINT arraySlice,
	UINT planeSlice)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = resDesc.Format;

	switch (resDesc.Dimension)
	{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if (resDesc.DepthOrArraySize > 1)
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
				uavDesc.Texture1DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
				uavDesc.Texture1DArray.FirstArraySlice = arraySlice;
				uavDesc.Texture1DArray.MipSlice = mipSlice;
			}
			else
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
				uavDesc.Texture1D.MipSlice = mipSlice;
			}
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (resDesc.DepthOrArraySize > 1)
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = resDesc.DepthOrArraySize - arraySlice;
				uavDesc.Texture2DArray.FirstArraySlice = arraySlice;
				uavDesc.Texture2DArray.PlaneSlice = planeSlice;
				uavDesc.Texture2DArray.MipSlice = mipSlice;
			}
			else
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.PlaneSlice = planeSlice;
				uavDesc.Texture2D.MipSlice = mipSlice;
			}
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.WSize = resDesc.DepthOrArraySize - arraySlice;
			uavDesc.Texture3D.FirstWSlice = arraySlice;
			uavDesc.Texture3D.MipSlice = mipSlice;
			break;
		default:
			throw std::exception("Invalid resource dimension.");
	}

	return uavDesc;
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

DXGI_FORMAT Texture::GetUAVCompatableFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT uavFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		uavFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
		uavFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	}

	return uavFormat;
}

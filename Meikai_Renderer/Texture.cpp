#include "Texture.h"

#include "DescriptorHeap.h"
#include "DXApp.h"
#include "DXUtil.h"

Texture::Texture(DXApp* mApp,
	const std::wstring& name)
	:Resource(mApp, name)
{
}

Texture::Texture(DXApp* mApp, ComPtr<ID3D12Resource> resource,  D3D12_SRV_DIMENSION srvDim,
	D3D12_UAV_DIMENSION uavDim, const std::wstring& name)
	:Resource(mApp, resource, name)
{
	CreateViews(srvDim, uavDim);
}

Texture::Texture(DXApp* mApp, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue,
	D3D12_SRV_DIMENSION srvDim, D3D12_UAV_DIMENSION uavDim, const std::wstring& name)
		:Resource(mApp, resourceDesc, clearValue, name)
{
	CreateViews(srvDim, uavDim);
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

void Texture::CreateViews(D3D12_SRV_DIMENSION srvDim, D3D12_UAV_DIMENSION uavDim)
{
	if(mResource)
	{
		auto device = mApp->GetDevice();
		CD3DX12_RESOURCE_DESC desc(mResource->GetDesc());
		if((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
		{
			mRTVDescIDX = mApp->GetDescriptorHeap(RTV)->GetNextAvailableIndex();
			mApp->CreateRtvDescriptor(desc.Format, mResource, mApp->GetHeapCPUHandle(HeapType::RTV, mRTVDescIDX.value()));
		}
		if((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
		{
			mDSVDescIDX = mApp->GetDescriptorHeap(DSV)->GetNextAvailableIndex();
			mApp->CreateDsvDescriptor(desc.Format, mResource, mApp->GetHeapCPUHandle(HeapType::DSV, mDSVDescIDX.value()));
		}
		if((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = GetUAVCompatableFormat(desc.Format);
			uavDesc.ViewDimension = uavDim;
			std::shared_ptr<DescriptorHeap> uavHeap = nullptr;
			switch (uavDim)
			{
				case D3D12_UAV_DIMENSION_TEXTURE2D:
					uavDesc.Texture2D.PlaneSlice = 0;
					uavDesc.Texture2D.MipSlice = 0;
					uavHeap = mApp->GetDescriptorHeap(UAV_2D);
					mUAVDescIDX = uavHeap->GetNextAvailableIndex();
					break;

				case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
					uavDesc.Texture2DArray.FirstArraySlice = 0;
					uavDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
					uavHeap = mApp->GetDescriptorHeap(UAV_2D_ARRAY);
					mUAVDescIDX = uavHeap->GetNextAvailableIndex();
					break;
			}

			mApp->CreateUavDescriptor(uavDesc, mResource, uavHeap->GetCpuHandle(mUAVDescIDX.value()));
		}
		if(srvDim != D3D12_SRV_DIMENSION_UNKNOWN)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = desc.Format;
			if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
			{
				srvDesc.Format = GetTypelessFormat(desc.Format);
			}
			srvDesc.ViewDimension = srvDim;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			std::shared_ptr<DescriptorHeap> srvHeap = nullptr;
			switch (srvDim)
			{
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.PlaneSlice = 0;
				srvHeap = mApp->GetDescriptorHeap(SRV_2D);
				mSRVDescIDX = srvHeap->GetNextAvailableIndex();
				break;

			case D3D12_SRV_DIMENSION_TEXTURECUBE:
				srvDesc.Texture2DArray.MipLevels = 1;
				//Current sky box has only 1 mip. And if set more than 1 MipLevel, Level 0 mip is blended with Level 1 mip which is (0, 0, 0) because it doesn't exist, and looks like occulusion.
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.PlaneSlice = 0;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
				srvHeap = mApp->GetDescriptorHeap(SRV_CUBE);
				mSRVDescIDX = srvHeap->GetNextAvailableIndex();
				break;
			}
			mApp->CreateSrvDescriptor(srvDesc, mResource, srvHeap->GetCpuHandle(mSRVDescIDX.value()));
		}
	}
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

DXGI_FORMAT Texture::GetTypelessFormat(DXGI_FORMAT format)
{
	DXGI_FORMAT typelessFormat = format;

	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32B32A32_TYPELESS;
		break;
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32B32_TYPELESS;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
		typelessFormat = DXGI_FORMAT_R16G16B16A16_TYPELESS;
		break;
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
		typelessFormat = DXGI_FORMAT_R32G32_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		typelessFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		typelessFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
		typelessFormat = DXGI_FORMAT_R10G10B10A2_TYPELESS;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		typelessFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
		break;
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
		typelessFormat = DXGI_FORMAT_R16G16_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
		typelessFormat = DXGI_FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
		typelessFormat = DXGI_FORMAT_R8G8_TYPELESS;
		break;
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		typelessFormat = DXGI_FORMAT_R16_TYPELESS;
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
		typelessFormat = DXGI_FORMAT_R8_TYPELESS;
		break;
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC1_TYPELESS;
		break;
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC2_TYPELESS;
		break;
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC3_TYPELESS;
		break;
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		typelessFormat = DXGI_FORMAT_BC4_TYPELESS;
		break;
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		typelessFormat = DXGI_FORMAT_BC5_TYPELESS;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
		break;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
		break;
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		typelessFormat = DXGI_FORMAT_BC6H_TYPELESS;
		break;
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		typelessFormat = DXGI_FORMAT_BC7_TYPELESS;
		break;
	}

	return typelessFormat;
}

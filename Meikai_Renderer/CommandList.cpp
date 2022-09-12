#include "CommandList.h"
#include "DXApp.h"
#include "DXUtil.h"
#include "ResourceStateTracker.h"
#include <DirectXTex.h>
#include <filesystem>

std::mutex CommandList::msTextureCacheMutex;
using namespace DirectX;

CommandList::CommandList(DXApp* appPtr, D3D12_COMMAND_LIST_TYPE type)
	:mApp(appPtr), mCommandListType(type)
{
	auto device = mApp->GetDevice();
	ThrowIfFailed(device->CreateCommandAllocator(mCommandListType, IID_PPV_ARGS(&mCommandAllocator)))
		ThrowIfFailed(device->CreateCommandList(0, mCommandListType, mCommandAllocator.Get(),
			nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())))

	mResourceStateTracker = std::make_unique<ResourceStateTracker>();
}

CommandList::~CommandList()
{

}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresources, bool flushBarriers)
{
	TransitionBarrier(resource.GetResource(), stateAfter, subresources, flushBarriers);
}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
	if (resource)
	{
		// The "before" state is not important. It will be resolved by the resource state tracker.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subresource);
		mResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
	CopyResource(dstRes.GetResource(), srcRes.GetResource());
}
void CommandList::CopyResource(ComPtr<ID3D12Resource> dstRes, ComPtr<ID3D12Resource> srcRes)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	mCommandList->CopyResource(dstRes.Get(), srcRes.Get());

	TrackResource(dstRes);
	TrackResource(srcRes);
}

void CommandList::FlushResourceBarriers()
{
	mResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage)
{
	std::filesystem::path filePath(fileName);
	if (std::filesystem::exists(filePath))
	{
		throw std::exception("File not found.");
	}

	std::lock_guard<std::mutex> lock(msTextureCacheMutex);

	TexMetadata metadata;
	ScratchImage scratchImage;

	if (filePath.extension() == ".dds")
	{
		ThrowIfFailed(LoadFromDDSFile(
			fileName.c_str(),
			DDS_FLAGS_NONE,
			&metadata,
			scratchImage));
	}
	else if (filePath.extension() == ".hdr")
	{
		ThrowIfFailed(LoadFromHDRFile(
			fileName.c_str(),
			&metadata,
			scratchImage));
	}
	else if (filePath.extension() == ".tga")
	{
		ThrowIfFailed(LoadFromTGAFile(
			fileName.c_str(),
			&metadata,
			scratchImage));
	}
	else
	{
		ThrowIfFailed(LoadFromWICFile(
			fileName.c_str(),
			WIC_FLAGS_NONE,
			&metadata,
			scratchImage));
	}

	D3D12_RESOURCE_DESC textureDesc = {};
	switch (metadata.dimension)
	{
	case TEX_DIMENSION_TEXTURE1D:
		textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT16>(metadata.arraySize));
		break;
	case TEX_DIMENSION_TEXTURE2D:
		textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.arraySize));
		break;
	case TEX_DIMENSION_TEXTURE3D:
		textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
			metadata.format,
			static_cast<UINT64>(metadata.width),
			static_cast<UINT>(metadata.height),
			static_cast<UINT16>(metadata.depth));
		break;
	default:
		throw std::exception("Invalid texture dimension.");
		break;
	}

	auto device = mApp->GetDevice();
	ComPtr<ID3D12Resource> textureResource;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&textureResource)))

	texture.SetTextureUsage(textureUsage);
	texture.SetD3D12Resource(textureResource);
	texture.SetName(fileName);

	ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

	std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
	const Image* pImages = scratchImage.GetImages();
	for (int i = 0; i < scratchImage.GetImageCount(); ++i)
	{
		auto& subresource = subresources[i];
		subresource.RowPitch = pImages[i].rowPitch;
		subresource.SlicePitch = pImages[i].slicePitch;
		subresource.pData = pImages[i].pixels;
	}

	CopyTextureSubresource(
		texture,
		0,
		static_cast<uint32_t>(subresources.size()),
		subresources.data());
}

void CommandList::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
{
	auto device = mApp->GetDevice();
	auto destResource = texture.GetResource();

	if (destResource)
	{
		TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushResourceBarriers();

		UINT64 requiredSize = GetRequiredIntermediateSize(destResource.Get(), firstSubresource, numSubresources);

		ComPtr<ID3D12Resource> intermediateResource;
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)
		))

		UpdateSubresources(mCommandList.Get(), destResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

		TrackResource(intermediateResource);
		TrackResource(destResource);
	}
}

void CommandList::Close(void)
{
	mCommandList->Close();
}

bool CommandList::Close(CommandList& pendingCommandList)
{
	FlushResourceBarriers();
	mCommandList->Close();
	uint32_t numPendingBarriers;
	//Resource Tracker something...

	return numPendingBarriers > 0;
}

void CommandList::Reset()
{
	ThrowIfFailed(mCommandAllocator->Reset())
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr))

	ReleaseTrackedObjects();
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	mCommandList->IASetPrimitiveTopology(primitiveTopology);
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	mCommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	mCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
	mCommandList->RSSetViewports(1, &viewport);
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
	mCommandList->RSSetScissorRects(1, &scissorRect);
}

void CommandList::SetPipelineState(ComPtr<ID3D12PipelineState> pipelineState)
{
	mCommandList->SetPipelineState(pipelineState.Get());
	TrackResource(pipelineState);
}

void CommandList::SetGraphicsRootSignature(ComPtr<ID3D12RootSignature> rootSignature)
{
	mCommandList->SetGraphicsRootSignature(rootSignature.Get());
}

void CommandList::ResourceBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	auto transition = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), before, after);
	mCommandList->ResourceBarrier(1, &transition);
}

void CommandList::TrackResource(const Resource& res)
{
	TrackResource(res.GetResource());
}

void CommandList::TrackResource(ComPtr<ID3D12Object> object)
{
	mTrackedObjects.push_back(object);
}

D3D12_COMMAND_LIST_TYPE CommandList::GetCommandListType()
{
	return mCommandListType;
}

void CommandList::ReleaseTrackedObjects()
{
	mTrackedObjects.clear(); 
}

ComPtr<ID3D12GraphicsCommandList2> CommandList::GetList()
{
	return mCommandList;
}

ComPtr<ID3D12CommandAllocator> CommandList::GetAllocator()
{
	return mCommandAllocator;
}
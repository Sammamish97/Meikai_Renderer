#pragma once
#include <cassert>
#include <d3dx12.h>
#include <wrl.h>
#include <vector>
#include "Texture.h"
#include <mutex>

class ResourceStateTracker;
class DXApp;
class Buffer;
class UploadBuffer;
class VertexBuffer;
class IndexBuffer;
using namespace Microsoft::WRL;
class CommandList
{
	DXApp* mApp;

public:
	CommandList(DXApp* appPtr, D3D12_COMMAND_LIST_TYPE type);
	~CommandList();

	CommandList(const CommandList& copy) = delete;
	CommandList& operator= (const CommandList& other) = delete;

public:
	void Close();
	bool Close(CommandList& pendingCommandList);
	void Reset();

	void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
	void TransitionBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

	void FlushResourceBarriers();
	void ResourceBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	void CopyResource(Resource& dstRes, const Resource& srcRes);
	void CopyResource(ComPtr<ID3D12Resource> dstRes, ComPtr<ID3D12Resource> srcRes);

	void CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	template<typename T>
	void CopyVertexBuffer(VertexBuffer& vertexBuffer, const std::vector<T>& vertexBufferData)
	{
		CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	void CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<typename T>
	void CopyIndexBuffer(IndexBuffer& indexBuffer, const std::vector<T>& indexBufferData)
	{
		assert(sizeof(T) == 2 || sizeof(T) == 4);

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	template<typename T>
	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
	{
		SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}

	void SetRootConstant(int rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS GPUAddress);
	void LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage = TextureUsage::Albedo);
	void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

	void ClearTexture(std::shared_ptr<Texture> texture, D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle, const float clearColor[4]);
	void ClearDepthStencilTexture(std::shared_ptr<Texture> texture, D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUHandle, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);

	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
	void SetRenderTargets(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& rtvArray, D3D12_CPU_DESCRIPTOR_HANDLE* dsvCPUHandle);

	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetScissorRect(const D3D12_RECT& scissorRect);
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	void SetGraphicsRootSignature(ComPtr<ID3D12RootSignature> rootSignature);

	void SetEmptyVertexBuffer();
	void SetEmptyIndexBuffer();
	void SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer);
	void SetIndexBuffer(const IndexBuffer& indexBuffer);

	void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

	ComPtr<ID3D12GraphicsCommandList2> GetList();
	ComPtr<ID3D12CommandAllocator> GetAllocator();
	D3D12_COMMAND_LIST_TYPE GetCommandListType();


	void SetDescriptorHeap(ComPtr<ID3D12DescriptorHeap>& heap);
	void SetDescriptorTable(UINT rootParamIndex, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle);
	void SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	template<typename T>
	void SetGraphics32BitConstants(uint32_t rootParameterIndex, const T& constants)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
		SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}
	void SetConstantBufferView(uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS dataGPUAddress);
	void ReleaseTrackedObjects();

private:
	void CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void TrackResource(ComPtr<ID3D12Object> object);
	void TrackResource(const Resource& res);
	
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> mCommandList;

	D3D12_COMMAND_LIST_TYPE mCommandListType;
	std::vector<ComPtr<ID3D12Object>> mTrackedObjects;

	std::unique_ptr<ResourceStateTracker> mResourceStateTracker;
	std::unique_ptr<UploadBuffer> mUploadBuffer;

	using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;
	TrackedObjects m_TrackedObjects;

	static std::mutex msTextureCacheMutex;
};


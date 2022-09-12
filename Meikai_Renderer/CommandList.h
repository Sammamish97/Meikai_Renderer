#pragma once
#include <d3dx12.h>
#include <wrl.h>
#include <vector>
#include "Texture.h"
#include <mutex>

class ResourceStateTracker;
class DXApp;
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

	//void CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	//void CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);

	//void CopyByteAddressBuffer(ByteAddressBuffer& byteAddressBuffer, size_t bufferSize, const void* bufferData);
	//void CopyStructuredBuffer(StructuredBuffer& structuredBuffer, size_t numElements, size_t elementSize, const void* bufferData);

	void LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage = TextureUsage::Albedo);
	void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

	void ClearTexture(const Texture& texture, const float clearColor[4]);
	void ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);

	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetScissorRect(const D3D12_RECT& scissorRect);
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	void SetGraphicsRootSignature(ComPtr<ID3D12RootSignature> rootSignature);

	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance);

	ComPtr<ID3D12GraphicsCommandList2> GetList();
	ComPtr<ID3D12CommandAllocator> GetAllocator();
	D3D12_COMMAND_LIST_TYPE GetCommandListType();

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

	void ReleaseTrackedObjects();

private:
	void TrackResource(ComPtr<ID3D12Object> object);
	void TrackResource(const Resource& res);
	
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> mCommandList;

	D3D12_COMMAND_LIST_TYPE mCommandListType;
	std::vector<ComPtr<ID3D12Object>> mTrackedObjects;

	std::unique_ptr<ResourceStateTracker> mResourceStateTracker;

	using TrackedObjects = std::vector < Microsoft::WRL::ComPtr<ID3D12Object> >;
	TrackedObjects m_TrackedObjects;

	static std::mutex msTextureCacheMutex;
};


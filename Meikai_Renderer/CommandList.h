#pragma once
#include <d3dx12.h>
#include <wrl.h>
#include <vector>
#include "Texture.h"

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

	void TransitionBarrier();
	void FlushResourceBarriers();
	void ResourceBarrier(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	void CopyResource(ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes);

	//void CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	//void CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);

	//void CopyByteAddressBuffer(ByteAddressBuffer& byteAddressBuffer, size_t bufferSize, const void* bufferData);
	//void CopyStructuredBuffer(StructuredBuffer& structuredBuffer, size_t numElements, size_t elementSize, const void* bufferData);

	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetScissorRect(const D3D12_RECT& scissorRect);
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	void SetGraphicsRootSignature(ComPtr<ID3D12RootSignature> rootSignature);

	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance);

	ComPtr<ID3D12GraphicsCommandList2> GetList();
	ComPtr<ID3D12CommandAllocator> GetAllocator();

private:
	void TrackResource(ComPtr<ID3D12Object> object);
	void ReleaseTrackedObjects();

private:
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> mCommandList;

	D3D12_COMMAND_LIST_TYPE mCommandListType;
	std::vector<ComPtr<ID3D12Object>> mTrackedObjects;
};


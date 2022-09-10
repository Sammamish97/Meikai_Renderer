#include "CommandList.h"
#include "DXApp.h"
#include "DXUtil.h"

CommandList::CommandList(DXApp* appPtr, D3D12_COMMAND_LIST_TYPE type)
	:mApp(appPtr), mCommandListType(type)
{
	auto device = mApp->GetDevice();
	ThrowIfFailed(device->CreateCommandAllocator(mCommandListType, IID_PPV_ARGS(&mCommandAllocator)))
	ThrowIfFailed(device->CreateCommandList(0, mCommandListType, mCommandAllocator.Get(),
		nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())))
}

CommandList::~CommandList()
{

}

void CommandList::Close()
{
	mCommandList->Close();
}

void CommandList::Reset()
{
	ThrowIfFailed(mCommandAllocator->Reset())
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr))

	ReleaseTrackedObjects();
}

void CommandList::CopyResource(ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes)
{

}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{

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

void CommandList::TrackResource(ComPtr<ID3D12Object> object)
{
	mTrackedObjects.push_back(object);
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
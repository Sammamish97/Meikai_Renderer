#include "CommandManager.h"
#include "DXApp.h"
#include "DXUtil.h"
CommandManager::CommandManager(DXApp* appPtr)
	:mApp(appPtr)
{
	InitTempCommandData();
}

void CommandManager::InitTempCommandData()
{
	mTempFenceValue = 0;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(mApp->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mTempQueue)))
	ThrowIfFailed(mApp->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mTempAllocator.GetAddressOf())))
	ThrowIfFailed(mApp->GetDevice()->CreateFence(mTempFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mTempFence.GetAddressOf())))
}

void CommandManager::AllocateTempList(ComPtr<ID3D12GraphicsCommandList2> cmdListPtr)
{
	ThrowIfFailed(mApp->GetDevice()->CreateCommandList
	(0, 
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mTempAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(cmdListPtr.GetAddressOf())))
	//After command list is created, it is opened state.
}

void CommandManager::FlushTempList(ComPtr<ID3D12GraphicsCommandList2> cmdListPtr)
{
	cmdListPtr->Close();
	++mTempFenceValue;
	ThrowIfFailed(mTempQueue->Signal(mTempFence.Get(), mTempFenceValue))
	if (mTempFence->GetCompletedValue() < mTempFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mTempFence->SetEventOnCompletion(mTempFenceValue, eventHandle))
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//Reset Allocator because it is temporary.
	mTempAllocator->Reset();
}
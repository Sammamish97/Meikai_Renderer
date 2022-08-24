#pragma once
#include <d3dx12.h>
#include <wrl.h>
class DXApp;
using namespace Microsoft::WRL;
class CommandManager
{
	DXApp* mApp;

public:
	CommandManager(DXApp* appPtr);
	void InitTempCommandData();
	void AllocateTempList(ComPtr<ID3D12GraphicsCommandList2>& cmdListPtr);
	void FlushTempList(ComPtr<ID3D12GraphicsCommandList2>& cmdListPtr);

private:
	ComPtr<ID3D12CommandQueue> mTempQueue;
	ComPtr<ID3D12CommandAllocator> mTempAllocator;
	ComPtr<ID3D12Fence> mTempFence;
	UINT64 mTempFenceValue = 0;
};


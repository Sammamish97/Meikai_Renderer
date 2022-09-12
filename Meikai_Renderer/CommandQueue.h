#pragma once
#include <d3dx12.h>
#include <wrl.h>
#include <atomic>
#include <cstdint>
#include <condition_variable>

#include "ThreadSafeQueue.h"

using namespace Microsoft::WRL;
class DXApp;
class CommandList;
class CommandQueue
{
	DXApp* mApp;
public:
	CommandQueue(DXApp* appPtr, D3D12_COMMAND_LIST_TYPE type);
	~CommandQueue();

	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	uint64_t ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList> >& commandLists);

	uint64_t Signal();
	bool IsFenceComplete();
	bool WaitForFenceValue(uint64_t fenceValue);
	void Flush();
	void Wait(const CommandQueue& other);
	std::shared_ptr<CommandList> GetCommandList();
	ComPtr<ID3D12CommandQueue> GetCommandQueue();

private:
	void ProcessInFlightCommandLists();

	using CommandListEntry = std::tuple<uint64_t, std::shared_ptr<CommandList>>;

	D3D12_COMMAND_LIST_TYPE mCommandListType;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12Fence> mFence;
	std::atomic_uint64_t mFenceValue;

	ThreadSafeQueue<CommandListEntry> mInFlightCommandLists;
	ThreadSafeQueue<std::shared_ptr<CommandList>> mAvailableCommandLists;

	std::thread mProcessInFlightCommandListsThread;
	std::atomic_bool mProcessInFlightCommandLists;
	std::mutex mProcessInFlightCommandListsThreadMutex;
	std::condition_variable mProcessInFlightCommandListsThreadCV;
};


#include "CommandQueue.h"

#include <cassert>

#include "DXApp.h"
#include "DXUtil.h"
#include "CommandList.h"

CommandQueue::CommandQueue(DXApp* appPtr, D3D12_COMMAND_LIST_TYPE type)
	:mApp(appPtr), mCommandListType(type), mProcessInFlightCommandLists(true)
{
	auto device = mApp->GetDevice();

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(mCommandQueue.GetAddressOf())))
    ThrowIfFailed(device->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mFence.GetAddressOf())))

    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_COPY:
        mCommandQueue->SetName(L"Copy Command Queue");
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        mCommandQueue->SetName(L"Compute Command Queue");
        break;
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        mCommandQueue->SetName(L"Direct Command Queue");
        break;
    }

    mProcessInFlightCommandListsThread = std::thread(&CommandQueue::ProcessInFlightCommandLists, this);
}

CommandQueue::~CommandQueue()
{
    mProcessInFlightCommandLists = false;
    mProcessInFlightCommandListsThread.join();
}

uint64_t CommandQueue::Signal()
{
    uint64_t fencevalue = ++mFenceValue;
    mCommandQueue->Signal(mFence.Get(), fencevalue);
    return fencevalue;
}

bool CommandQueue::IsFenceComplete()
{
    return mFence->GetCompletedValue() >= mFenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if(!IsFenceComplete())
    {
        auto event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(event && "Failed to create fence event handle");

        mFence->SetEventOnCompletion(fenceValue, event);
        ::WaitForSingleObject(event, DWORD_MAX);
        ::CloseHandle(event);
    }
}

void CommandQueue::Flush()
{
    std::unique_lock<std::mutex> lock(mProcessInFlightCommandListsThreadMutex);
    mProcessInFlightCommandListsThreadCV.wait(lock, [this] { return mInFlightCommandLists.Empty(); });
    WaitForFenceValue(mFenceValue);
}

std::shared_ptr<CommandList> CommandQueue::GetCommandList()
{
    std::shared_ptr<CommandList> commandList;
    if(mAvailableCommandLists.Empty() == false)
    {
        mAvailableCommandLists.TryPop(commandList);
    }
    else
    {
        commandList = std::make_shared<CommandList>(mApp, mCommandListType);
    }

    return commandList;
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue()
{
    return mCommandQueue;
}


uint64_t CommandQueue::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
    return ExecuteCommandLists(std::vector<std::shared_ptr<CommandList>>({commandList}));
}

uint64_t CommandQueue::ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& commandLists)
{
    //Command lists that will be recycle.
    std::vector<std::shared_ptr<CommandList>> toBeQueued;
    toBeQueued.reserve(commandLists.size() * 2);

    std::vector<ID3D12CommandList*> toBeExecute;
    toBeExecute.reserve(commandLists.size() * 2);

    for(auto commandList : commandLists)
    {
        auto pendingCommandList = GetCommandList();
        bool hasPendingBarriers = commandList->Close(*pendingCommandList);
        pendingCommandList->Close();
        if(hasPendingBarriers)
        {
            toBeExecute.push_back(pendingCommandList->GetList().Get());
        }
        toBeExecute.push_back(commandList->GetList().Get());

        toBeQueued.push_back(pendingCommandList);
        toBeQueued.push_back(commandList);
    }

    UINT numCommandLists = static_cast<UINT>(toBeExecute.size());
    mCommandQueue->ExecuteCommandLists(numCommandLists, toBeExecute.data());
    uint64_t fenceValue = Signal();

    for(auto commandList : toBeQueued)
    {
        mInFlightCommandLists.Push({ fenceValue, commandList });
    }
    return fenceValue;
}

void CommandQueue::Wait(const CommandQueue& other)
{
    mCommandQueue->Wait(other.mFence.Get(), other.mFenceValue);
}

void CommandQueue::ProcessInFlightCommandLists()
{
    std::unique_lock<std::mutex> lock(mProcessInFlightCommandListsThreadMutex, std::defer_lock);
    

    while(mProcessInFlightCommandLists)
    {
        CommandListEntry commandListEntry;
        lock.lock();
        while (mInFlightCommandLists.TryPop(commandListEntry));
        {
            auto fenceValue = std::get<0>(commandListEntry);
            auto commandList = std::get<1>(commandListEntry);

            WaitForFenceValue(fenceValue);

            commandList->Reset();

            mAvailableCommandLists.Push(commandList);
        }
        lock.unlock();
        mProcessInFlightCommandListsThreadCV.notify_one();
        std::this_thread::yield();
        
    }
}

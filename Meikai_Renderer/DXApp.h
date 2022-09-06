#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <d3dx12.h>
#include <array>
#include <memory>

#include "CommandManager.h"
#include "GameTimer.h"
#include "ResourceAllocator.h"

using Microsoft::WRL::ComPtr;

class DXApp
{
protected:
    DXApp(HINSTANCE hInstance);
    DXApp(const DXApp& rhs) = delete;
    DXApp& operator=(const DXApp& rhs) = delete;
    virtual ~DXApp();

public:
    static DXApp* GetApp();
    HINSTANCE AppInst()const;
    HWND      MainWnd()const;
    ComPtr<ID3D12Device2> GetDevice();
    float     AspectRatio()const;

    bool Get4xMsaaState();
    void Set4xMsaaState(bool value);
    UINT DXApp::Get4xMsaaQuality();
    std::pair<int, int> GetWindowSize();

    UINT GetRtvDescSize();
    UINT GetDsvDescSize();
    UINT GetCbvSrvUavDescSize();

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

    int Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void CreateFence();

protected:
    virtual void CreateSwapChainRtvDescriptorHeap();
    virtual void OnResize();
    virtual void Update(const GameTimer& gt) = 0;
    virtual void Draw(const GameTimer& gt) = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
    virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
    virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
    bool InitMainWindow();
    bool InitDirect3D();
    void CreateCommandObjects();
    void CreateSwapChain();

    void FlushCommandQueue();

    ID3D12Resource* CurrentBackBuffer()const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferExtView() const;

    void CalculateFrameStats();

    void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
        ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
    
    void ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);
   
    void ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

    void Create2DTextureResource(ComPtr<ID3D12Resource>& destination, int width, int height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flag);
    void CreateCubemapTextureResource(ComPtr<ID3D12Resource>& destination, int width, int height, DXGI_FORMAT format);

    void Load2DTextureFromFile(ComPtr<ID3D12Resource>& destination, const std::wstring& path, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flag);
    void LoadHDRTextureFromFile(ComPtr<ID3D12Resource>& destination, const std::wstring& path, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flag);

    void CopyBufferToTexture(void* data, ComPtr<ID3D12Resource>& destination, int width, int height, size_t elementSize, DXGI_FORMAT format);

    void CreateRtvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
    void CreateDsvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
    void CreateCbvDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuLocation, size_t bufferSize, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
    void CreateSrvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);

    //void TransitionBarrier()
    //void CreateUavDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);

public:
    //Update buffer by buffer
    void UpdateDefaultBufferResource(
        ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    //Upldate texture by buffer
    void UpdateDefaultTextureResource(
        ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        DXGI_FORMAT format, int width, int height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

private:
    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

public:
    // Manage Command list things.
    std::unique_ptr<CommandManager> mCommandMgr;
    std::unique_ptr<ResourceAllocator> mResourceAllocator;

protected:
    static DXApp* mApp;

    HINSTANCE mhAppInst = nullptr; // application instance handle
    HWND      mhMainWnd = nullptr; // main window handle
    bool      mAppPaused = false;  // is the application paused?
    bool      mMinimized = false;  // is the application minimized?
    bool      mMaximized = false;  // is the application maximized?
    bool      mResizing = false;   // are the resize bars being dragged?
    bool      mFullscreenState = false;// fullscreen enabled

    // Set true to use 4X MSAA (?.1.8).  The default is false.
    bool      m4xMsaaState = false;    // 4X MSAA enabled
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA

    GameTimer mTimer;

    ComPtr<IDXGIFactory4> mdxgiFactory;
    ComPtr<IDXGISwapChain> mSwapChain;
    ComPtr<ID3D12Device2> mdxDevice;

    ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList2> mCommandList;

    static const int SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;
    ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    ComPtr<ID3D12DescriptorHeap> mSwapChainRtvHeap;

    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    // Derived class should set these in derived constructor to customize starting values.
    std::wstring mMainWndCaption = L"dx App";
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

    int mClientWidth = 800;
    int mClientHeight = 600;
};


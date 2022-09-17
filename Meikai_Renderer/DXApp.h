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

#include "GameTimer.h"
#include "ResourceAllocator.h"


using Microsoft::WRL::ComPtr;
class CommandList;
class CommandQueue;
class Texture;

class DXApp
{
protected:
    DXApp(HINSTANCE hInstance);
    DXApp(const DXApp& rhs) = delete;
    DXApp& operator=(const DXApp& rhs) = delete;
    virtual ~DXApp();

public:
    static DXApp* GetApp();
    std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);
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
    void Present(std::shared_ptr<Texture>& texture);
   
    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateSwapChainRtvDescriptorHeap();
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
    void CacheSwapChainImage();

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

    void CreateCubemapTextureResource(ComPtr<ID3D12Resource>& destination, int width, int height, DXGI_FORMAT format);

    void CreateRtvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
    void CreateDsvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
    void CreateCbvDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuLocation, size_t bufferSize, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
    void CreateSrvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);

private:
    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);
    ComPtr<ID3D12Resource> stagingResource;

protected:
    std::shared_ptr<CommandQueue> mDirectCommandQueue;
    std::shared_ptr<CommandQueue> mComputeCommandQueue;
    std::shared_ptr<CommandQueue> mCopyCommandQueue;

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

    static const int SwapChainBufferCount = 2;
    int mCurrBackBuffer = 0;
    ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    ComPtr<ID3D12DescriptorHeap> mSwapChainRtvHeap;
    UINT64 mFenceValues[SwapChainBufferCount] = {0};

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


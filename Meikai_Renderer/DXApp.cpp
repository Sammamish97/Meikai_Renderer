#include "DXApp.h"
#include "DXUtil.h"
#include "BufferFormat.h"

#include "CommandList.h"
#include "CommandQueue.h"
#include "UploadBuffer.h"
#include "Texture.h"
#include "DescriptorHeap.h"
#include "ResourceStateTracker.h"


#include <d3dx12.h>
#include <cassert>
#include <vector>
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return DXApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

DXApp* DXApp::mApp = nullptr;

DXApp::DXApp(HINSTANCE hInstance)
	:mhAppInst(hInstance)
{
	assert(mApp == nullptr);
	mApp = this;
}

DXApp::~DXApp()
{
}

DXApp* DXApp::GetApp()
{
	return mApp;
}

std::shared_ptr<CommandQueue> DXApp::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	std::shared_ptr<CommandQueue> commandQueue;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = mDirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = mComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = mCopyCommandQueue;
		break;
	default:
		assert(false && "Invalid command queue type.");
	}

	return commandQueue;
}

HINSTANCE DXApp::AppInst() const
{
	return mhAppInst;
}

HWND DXApp::MainWnd() const
{
	return mhMainWnd;
}

ComPtr<ID3D12Device2> DXApp::GetDevice()
{
	return mdxDevice;
}

float DXApp::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool DXApp::Get4xMsaaState()
{
	return m4xMsaaState;
}

UINT DXApp::Get4xMsaaQuality()
{
	return m4xMsaaQuality;
}

std::pair<int, int> DXApp::GetWindowSize()
{
	return std::make_pair(mClientWidth, mClientHeight);
}

GameTimer DXApp::GetTimer()
{
	return mTimer;
}

UINT DXApp::GetRtvDescSize()
{
	return mRtvDescriptorSize;
}

UINT DXApp::GetDsvDescSize()
{
	return mDsvDescriptorSize;
}

UINT DXApp::GetCbvSrvUavDescSize()
{
	return mCbvSrvUavDescriptorSize;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 8> DXApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(
		4, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		6, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		7, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,linearRepeatSampler,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

void DXApp::Set4xMsaaState(bool value)
{
	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		// Recreate the swapchain and buffers with new multisample settings.
		CreateSwapChain();
	}
}

int DXApp::Run()
{
	MSG msg = {0};
 
	mTimer.Reset();

	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		else
        {	
			mTimer.Tick();

			if( !mAppPaused )
			{
				CalculateFrameStats();
				Update(mTimer);	
                Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

bool DXApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	InitCommonTextures();
	return true;
}

void DXApp::InitCommonTextures()
{
	CreateBufferResources();
	CacheTextureIndices();
}

void DXApp::CreateSwapChainRtvDescriptorHeap()
{
	//Create RTV descriptor heap for swap chain color images
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//TODO: For texture techniques, maybe flag need to be SHADER_VISIBLE.
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdxDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mSwapChainRtvHeap.GetAddressOf())))
}

bool DXApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool DXApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))
	debugController->EnableDebugLayer();
}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)))

	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,//Default adapter
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(&mdxDevice));

	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)))

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mdxDevice)))
	}

	mRtvDescriptorSize = mdxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mdxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
 	mCbvSrvUavDescriptorSize = mdxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(mdxDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)))

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	//Initial swap chain image's state is common | present.
	CreateCommandObjects();
	CreateSwapChain();
	CreateSwapChainRtvDescriptorHeap();
	CacheSwapChainImage();
	CreateDescriptorHeaps();
	return true;
}

void DXApp::CreateCommandObjects()
{
	mDirectCommandQueue = std::make_unique<CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeCommandQueue = std::make_unique<CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mCopyCommandQueue = std::make_unique<CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_COPY);
}

void DXApp::CreateSwapChain()
{
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferDesc.Width = mClientWidth;
	swapChainDesc.BufferDesc.Height = mClientHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = BackBufferFormat;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = SwapChainBufferCount;
	swapChainDesc.OutputWindow = mhMainWnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	mdxgiFactory->CreateSwapChain(mDirectCommandQueue->GetCommandQueue().Get(), &swapChainDesc, mSwapChain.GetAddressOf());
}

void DXApp::CacheSwapChainImage()
{
	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		mSwapChainBuffer[i].Reset();
	}

	for (UINT i = 0; i < SwapChainBufferCount; ++i)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])))
	}
}

ID3D12Resource* DXApp::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DXApp::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mSwapChainRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DXApp::CurrentBackBufferExtView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mSwapChainRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

void DXApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void DXApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, BackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void DXApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void DXApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DXApp::Present(std::shared_ptr<Texture>& texture)
{
	auto cmdList = mDirectCommandQueue->GetCommandList();
	if(texture->IsValid())
	{
		if(texture->GetD3D12ResourceDesc().SampleDesc.Count>1)
		{
			//If texture is multi sampled, need to Resolve.
		}
		else
		{
			cmdList->CopyResource(CurrentBackBuffer(), texture->GetResource());
		}
	}

	cmdList->TransitionBarrier(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
	mDirectCommandQueue->ExecuteCommandList(cmdList);

	//UINT syncInterval = m_VSync ? 1 : 0;
	//UINT presentFlags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(mSwapChain->Present(false, 0))
	mFenceValues[mCurrBackBuffer] = mDirectCommandQueue->Signal();

	mDirectCommandQueue->WaitForFenceValue(mFenceValues[mCurrBackBuffer]);
}

UINT DXApp::GetHeapDescriptorNum(HeapType type)
{
	return mDescriptorHeaps[type]->GetDescriptorNum();
}

std::shared_ptr<DescriptorHeap> DXApp::GetDescriptorHeap(HeapType type)
{
	return mDescriptorHeaps[type];
}

D3D12_CPU_DESCRIPTOR_HANDLE DXApp::GetHeapCPUHandle(HeapType type, UINT idx)
{
	return mDescriptorHeaps[type]->GetCpuHandle(idx);
}

D3D12_GPU_DESCRIPTOR_HANDLE DXApp::GetHeapGPUHandle(HeapType type, UINT idx)
{
	return mDescriptorHeaps[type]->GetGpuHandle(idx);
}

void DXApp::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
                               Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState,
                               D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

void DXApp::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv,
	FLOAT* clearColor)
{
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void DXApp::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv,
	FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void DXApp::CreateRtvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	mdxDevice->CreateRenderTargetView(resource.Get(), &rtvDesc, heapPos);
}

void DXApp::CreateDsvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource>& resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = format;
	dsvDesc.Texture2D.MipSlice = 0;
	mdxDevice->CreateDepthStencilView(resource.Get(), &dsvDesc, heapPos);
}

void DXApp::CreateCbvDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuLocation, size_t bufferSize, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = gpuLocation;
	cbvDesc.SizeInBytes = bufferSize;
	mdxDevice->CreateConstantBufferView(&cbvDesc, heapPos);
}

void DXApp::CreateSrvDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, ComPtr<ID3D12Resource>& resource,
	D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
	mdxDevice->CreateShaderResourceView(resource.Get(), &srvDesc, heapPos);
}

void DXApp::CreateUavDescriptor(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, ComPtr<ID3D12Resource>& resource,
                                D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
	mdxDevice->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, heapPos);
}

void DXApp::CreateDescriptorHeaps()
{
	mDescriptorHeaps[HeapType::RTV] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mRtvDescriptorSize, 128);
	mDescriptorHeaps[HeapType::DSV] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, mDsvDescriptorSize, 128);
	mDescriptorHeaps[HeapType::SRV_1D] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 128);
	mDescriptorHeaps[HeapType::SRV_2D] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 1024);
	mDescriptorHeaps[HeapType::SRV_CUBE] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 128);
	mDescriptorHeaps[HeapType::UAV_2D] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 128);
	mDescriptorHeaps[HeapType::UAV_2D_ARRAY] = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 128);
}

void DXApp::CreateBufferResources()
{
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(AlbedoFormat, mClientWidth, mClientHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto posDesc = CD3DX12_RESOURCE_DESC::Tex2D(PositionFormat, mClientWidth, mClientHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto normalDesc = CD3DX12_RESOURCE_DESC::Tex2D(NormalFormat, mClientWidth, mClientHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto monoDesc = CD3DX12_RESOURCE_DESC::Tex2D(MonoFormat, mClientWidth, mClientHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DepthStencilDSVFormat, mClientWidth, mClientHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	CD3DX12_CLEAR_VALUE clearColorBlack;
	clearColorBlack.Color[0] = 0.f;
	clearColorBlack.Color[1] = 0.f;
	clearColorBlack.Color[2] = 0.f;
	clearColorBlack.Color[3] = 0.f;

	CD3DX12_CLEAR_VALUE clearAlbedo = clearColorBlack;
	clearAlbedo.Format = AlbedoFormat;

	CD3DX12_CLEAR_VALUE clearPos = clearColorBlack;
	clearPos.Format = PositionFormat;

	CD3DX12_CLEAR_VALUE clearNormal = clearColorBlack;
	clearNormal.Format = NormalFormat;

	CD3DX12_CLEAR_VALUE clearMetalicRoughnessSSAO = clearColorBlack;
	clearMetalicRoughnessSSAO.Format = MonoFormat;

	CD3DX12_CLEAR_VALUE clearColorDepth;
	clearColorDepth.Format = DepthStencilDSVFormat;
	clearColorDepth.DepthStencil = { 1.f, 0 };

	mFrameResource.mPositionMap = std::make_shared<Texture>(this, posDesc, &clearPos, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"Pos");
	mFrameResource.mNormalMap = std::make_shared<Texture>(this, normalDesc, &clearNormal, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"Normal");
	mFrameResource.mAlbedoMap = std::make_shared<Texture>(this, colorDesc, &clearAlbedo,  D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"Albedo");
	mFrameResource.mRoughnessMap = std::make_shared<Texture>(this, monoDesc, &clearMetalicRoughnessSSAO,  D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"Roughness");
	mFrameResource.mMetalicMap = std::make_shared<Texture>(this, monoDesc, &clearMetalicRoughnessSSAO, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"Metalic");
	mFrameResource.mSsaoMap = std::make_shared<Texture>(this, monoDesc, &clearMetalicRoughnessSSAO,  D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"SSAO");
	mFrameResource.mDepthStencilBuffer = std::make_shared<Texture>(this, depthDesc, &clearColorDepth, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"DepthStencil");
	mFrameResource.mRenderTarget = std::make_shared<Texture>(this, colorDesc, &clearAlbedo, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, L"RenderTarget");

	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mPositionMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mNormalMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mAlbedoMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mMetalicMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mRoughnessMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mSsaoMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mDepthStencilBuffer->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mRenderTarget->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
}

void DXApp::CacheTextureIndices()
{
	mDescIndex.mPositionDescRtvIdx = mFrameResource.mPositionMap->mRTVDescIDX.value();
	mDescIndex.mNormalDescRtvIdx = mFrameResource.mNormalMap->mRTVDescIDX.value();
	mDescIndex.mAlbedoDescRtvIdx = mFrameResource.mAlbedoMap->mRTVDescIDX.value();
	mDescIndex.mRoughnessDescRtvIdx = mFrameResource.mRoughnessMap->mRTVDescIDX.value();
	mDescIndex.mMetalicDescRtvIdx = mFrameResource.mMetalicMap->mRTVDescIDX.value();
	mDescIndex.mSsaoDescRtvIdx = mFrameResource.mSsaoMap->mRTVDescIDX.value();
	mDescIndex.mRenderTargetRtvIdx = mFrameResource.mRenderTarget->mRTVDescIDX.value();

	mDescIndex.mPositionDescSrvIdx = mFrameResource.mPositionMap->mSRVDescIDX.value();
	mDescIndex.mNormalDescSrvIdx = mFrameResource.mNormalMap->mSRVDescIDX.value();
	mDescIndex.mAlbedoDescSrvIdx = mFrameResource.mAlbedoMap->mSRVDescIDX.value();
	mDescIndex.mRoughnessDescSrvIdx = mFrameResource.mRoughnessMap->mSRVDescIDX.value();
	mDescIndex.mMetalicDescSrvIdx = mFrameResource.mMetalicMap->mSRVDescIDX.value();
	mDescIndex.mSsaoDescSrvIdx = mFrameResource.mSsaoMap->mSRVDescIDX.value();
	mDescIndex.mDepthStencilSrvIdx = mFrameResource.mDepthStencilBuffer->mSRVDescIDX.value();

	mDescIndex.mDepthStencilDsvIdx = mFrameResource.mDepthStencilBuffer->mDSVDescIDX.value();
}

LRESULT DXApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (mdxDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "Camera.h"

#include "Texture.h"
#include "MathHelper.h"
#include "DefaultPass.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

//Cube model data

Demo::Demo(HINSTANCE hInstance)
	:DXApp(hInstance)
{
}

Demo::~Demo()
{
}

bool Demo::Initialize()
{
	if(DXApp::Initialize() == false)
	{
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	LoadContent();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	CreateDepthStencilData();
	mDefaultPass = std::make_unique<DefaultPass>(this, mShaders["DefaultForwardVS"], mShaders["DefaultForwardPS"]);
	

	OnResize();
	return true;
}

void Demo::OnResize()
{
	DXApp::OnResize();
	
	FlushCommandQueue();
}

void Demo::LoadContent()
{
	float aspectRatio = mClientWidth / static_cast<float>(mClientHeight);
	mCamera = std::make_unique<Camera>(aspectRatio);
	BuildModels();
	BuildFrameResource();
	CreateShader();
	
	m_ContentLoaded = true; 
}

void Demo::CreateDepthStencilData()
{
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = GetWindowSize().first;
	depthStencilDesc.Height = GetWindowSize().second;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  s
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = Get4xMsaaState() ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = Get4xMsaaState() ? (Get4xMsaaQuality() - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DepthStencilFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(mdxDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	mdxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf()));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	mdxDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Demo::BuildModels()
{
	mModels["Monkey"] = std::make_shared<Model>("../models/Monkey.obj", this, mCommandList);
	mModels["Quad"] = std::make_shared<Model>("../models/Quad.obj", this, mCommandList);
	mModels["Torus"] = std::make_shared<Model>("../models/Torus.obj", this, mCommandList);
	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", this, mCommandList);
	mModels["Skybox"] = std::make_shared<Model>("../models/Skybox.obj", this, mCommandList);

	objects.push_back(std::make_unique<Object>(mModels["Plane"], XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(10.f, 10.f, 10.f)));
	objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(0.f, 0.f, 0.f)));

	mSkybox = std::make_unique<Object>(mModels["Skybox"], XMFLOAT3(0.f, 0.f, 0.f));
	//objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(1.f, -1.f, 0.f)));
}

void Demo::BuildFrameResource()
{
	const size_t ConstantBufferAlignment = 256;

	mPassCB = std::make_unique<PassCB>();
	mPassAllocation = mResourceAllocator->AllocateToUploadHeap(&mPassCB, sizeof(PassCB), ConstantBufferAlignment);
	
	mLightCB = std::make_unique<LightCB>();
	mLightAllocation = mResourceAllocator->AllocateToUploadHeap(&mLightCB, sizeof(LightCB), ConstantBufferAlignment);

	mTestDeafult = mResourceAllocator->AllocateToDefaultHeap(&mPassCB, sizeof(PassCB), ConstantBufferAlignment);
}

void Demo::CreateShader()
{
	mShaders["standardVS"] = DxUtil::CompileShader(L"../shaders/VertexShader.hlsl", nullptr, "main", "vs_5_1");
	mShaders["opaquePS"] = DxUtil::CompileShader(L"../shaders/PixelShader.hlsl", nullptr, "main", "ps_5_1");
	mShaders["GeomVS"] = DxUtil::CompileShader(L"../shaders/GeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["GeomPS"] = DxUtil::CompileShader(L"../shaders/GeometryPass.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["ScreenQuadVS"] = DxUtil::CompileShader(L"../shaders/ScreenQuad.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["LightingPS"] = DxUtil::CompileShader(L"../shaders/LightingPass.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["SsaoPS"] = DxUtil::CompileShader(L"../shaders/SsaoPass.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["HBlurCS"] = DxUtil::CompileShader(L"../shaders/Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_1");
	mShaders["VBlurCS"] = DxUtil::CompileShader(L"../shaders/Blur.hlsl", nullptr, "VertBlurCS", "cs_5_1");
	mShaders["DefaultForwardVS"] = DxUtil::CompileShader(L"../shaders/DefaultForward.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["DefaultForwardPS"] = DxUtil::CompileShader(L"../shaders/DefaultForward.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SkyboxVS"] = DxUtil::CompileShader(L"../shaders/SkyboxPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyboxPS"] = DxUtil::CompileShader(L"../shaders/SkyboxPass.hlsl", nullptr, "PS", "ps_5_1");
}

void Demo::UpdatePassCB(const GameTimer& gt)
{
	PassCB currentFrameCB;

	XMMATRIX view = XMLoadFloat4x4(&mCamera->GetViewMat());
	XMMATRIX proj = XMLoadFloat4x4(&mCamera->GetProjMat());

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&currentFrameCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&currentFrameCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&currentFrameCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&currentFrameCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&currentFrameCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&currentFrameCB.InvViewProj, XMMatrixTranspose(invViewProj));

	currentFrameCB.EyePosW = mCamera->GetPosition();
	currentFrameCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	currentFrameCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	currentFrameCB.NearZ = 1.0f;
	currentFrameCB.FarZ = 1000.0f;
	currentFrameCB.TotalTime = gt.TotalTime();
	currentFrameCB.DeltaTime = gt.DeltaTime();

	mPassAllocation.Copy(&currentFrameCB, sizeof(PassCB));
}

void Demo::UpdateLightCB(const GameTimer& gt)
{
	LightCB lightData;

	lightData.directLight.Direction = XMFLOAT3(-0.5f, 0.5f, 0.5f);
	lightData.directLight.Color = XMFLOAT3(0.1f, 0.1f, 0.1f);

	lightData.pointLight[0].Position = XMFLOAT3(2, 0, 0);
	lightData.pointLight[0].Color = XMFLOAT3(1, 0, 0);

	lightData.pointLight[1].Position = XMFLOAT3(0, 2, 0);
	lightData.pointLight[1].Color = XMFLOAT3(0.0, 1, 0.0);

	lightData.pointLight[2].Position = XMFLOAT3(0, 0, 2);
	lightData.pointLight[2].Color = XMFLOAT3(0, 0, 1);

	/*auto LightCB = L_Pass->mLightCB.get();
	LightCB->CopyData(0, lightData);*/
}

void Demo::Update(const GameTimer& gt)
{
	mCamera->Update(gt);
	UpdatePassCB(gt);
	UpdateLightCB(gt);
}

void Demo::Draw(const GameTimer& gt)
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));


	DrawDefaultPass();

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Wait until frame commands are complete.  This waiting is inefficient and is
	// done for simplicity.  Later we will show how to organize our rendering code
	// so we do not have to wait per frame.
	FlushCommandQueue();
}

void Demo::DrawDefaultPass()
{
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float colorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mCommandList->SetPipelineState(mDefaultPass->mPSO.Get());
	mCommandList->SetGraphicsRootSignature(mDefaultPass->mRootSig.Get());

	//Remove it with bindless later.
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = mPassAllocation.GPU;
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), colorClearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvArray = {CurrentBackBufferExtView()};
	mCommandList->OMSetRenderTargets(rtvArray.size(), rtvArray.data(), true, &mDsvHeap->GetCPUDescriptorHandleForHeapStart());

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : objects)
	{
		object->Draw(mCommandList);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void Demo::OnMouseDown(WPARAM btnState, int x, int y)
{
	DXApp::OnMouseDown(btnState, x, y);
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Demo::OnMouseUp(WPARAM btnState, int x, int y)
{
	DXApp::OnMouseUp(btnState, x, y);
	ReleaseCapture();
}

void Demo::OnMouseMove(WPARAM btnState, int x, int y)
{
	DXApp::OnMouseMove(btnState, x, y);
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mCamera->mTheta += dx;
		mCamera->mPhi += dy;

		// Restrict the angle mPhi.
		mCamera->mPhi = MathHelper::Clamp(mCamera->mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mCamera->mRadius += dx - dy;

		// Restrict the radius.
		mCamera->mRadius = MathHelper::Clamp(mCamera->mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

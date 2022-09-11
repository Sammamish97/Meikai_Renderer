﻿#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "Camera.h"
#include "BufferFormat.h"
#include "MathHelper.h"
#include "DefaultPass.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "GeometryPass.h"
#include "LightingPass.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


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

	CreateDescriptorHeaps();

	CreateBufferResources();
	CreateBufferDescriptors();

	CreateIBLResources();
	CreateIBLDescriptors();

	LoadContent();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
	mDefaultPass = std::make_unique<DefaultPass>(this, mShaders["DefaultForwardVS"], mShaders["DefaultForwardPS"]);
	mGeometryPass = std::make_unique<GeometryPass>(this, mShaders["GeomVS"], mShaders["GeomPS"]);
	mLightingPass = std::make_unique<LightingPass>(this, mShaders["ScreenQuadVS"], mShaders["LightingPS"]);

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

void Demo::CreateDescriptorHeaps()
{
	mRTVHeap = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mRtvDescriptorSize, 128);
	mDSVHeap = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, mDsvDescriptorSize, 128);
	mCBVSRVUAVHeap = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 128);
}

void Demo::CreateBufferResources()
{
	Create2DTextureResource(mFrameResource.mPositionMap, mClientWidth, mClientHeight, PositionFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Create2DTextureResource(mFrameResource.mNormalMap, mClientWidth, mClientHeight, NormalFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Create2DTextureResource(mFrameResource.mAlbedoMap, mClientWidth, mClientHeight, AlbedoFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Create2DTextureResource(mFrameResource.mMetalicMap, mClientWidth, mClientHeight, MetalicFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Create2DTextureResource(mFrameResource.mRoughnessMap, mClientWidth, mClientHeight, RoughnessFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	Create2DTextureResource(mFrameResource.mDepthStencilBuffer, mClientWidth, mClientHeight, DepthStencilDSVFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
}


void Demo::CreateBufferDescriptors()
{
	mDescIndex.mPositionDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mNormalDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mAlbedoDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mRoughnessDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mMetalicDescRtvIdx = mRTVHeap->GetNextAvailableIndex();

	mDescIndex.mPositionDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mNormalDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mAlbedoDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mRoughnessDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mMetalicDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mDepthStencilSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();

	mDescIndex.mDepthStencilDsvIdx = mDSVHeap->GetNextAvailableIndex();

	CreateRtvDescriptor(PositionFormat, mFrameResource.mPositionMap, mRTVHeap->GetCpuHandle(mDescIndex.mPositionDescRtvIdx));
	CreateRtvDescriptor(NormalFormat, mFrameResource.mNormalMap, mRTVHeap->GetCpuHandle(mDescIndex.mNormalDescRtvIdx));
	CreateRtvDescriptor(AlbedoFormat, mFrameResource.mAlbedoMap, mRTVHeap->GetCpuHandle(mDescIndex.mAlbedoDescRtvIdx));
	CreateRtvDescriptor(RoughnessFormat, mFrameResource.mRoughnessMap, mRTVHeap->GetCpuHandle(mDescIndex.mRoughnessDescRtvIdx));
	CreateRtvDescriptor(MetalicFormat, mFrameResource.mMetalicMap, mRTVHeap->GetCpuHandle(mDescIndex.mMetalicDescRtvIdx));

	CreateSrvDescriptor(PositionFormat, mFrameResource.mPositionMap, mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mPositionDescSrvIdx));
	CreateSrvDescriptor(NormalFormat, mFrameResource.mNormalMap, mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mNormalDescSrvIdx));
	CreateSrvDescriptor(AlbedoFormat, mFrameResource.mAlbedoMap, mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mAlbedoDescSrvIdx));
	CreateSrvDescriptor(RoughnessFormat, mFrameResource.mRoughnessMap, mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mRoughnessDescSrvIdx));
	CreateSrvDescriptor(MetalicFormat, mFrameResource.mMetalicMap, mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mMetalicDescSrvIdx));
	CreateSrvDescriptor(DepthStencilSRVFormat, mFrameResource.mDepthStencilBuffer, mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mDepthStencilSrvIdx));

	CreateDsvDescriptor(DepthStencilDSVFormat, mFrameResource.mDepthStencilBuffer, mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx));
}

void Demo::CreateIBLResources()
{
	LoadHDRTextureFromFile(mIBLResource.mHDRImage, L"../textures/Alexs_Apt_2k.hdr", HDRFormat, D3D12_RESOURCE_FLAG_NONE);
}

void Demo::CreateIBLDescriptors()
{
	mIBLIndex.mHDRImageSrvIndex = mCBVSRVUAVHeap->GetNextAvailableIndex();
	CreateSrvDescriptor(HDRFormat, mIBLResource.mHDRImage, mCBVSRVUAVHeap->GetCpuHandle(mIBLIndex.mHDRImageSrvIndex));
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

	mCommonCB = std::make_unique<CommonCB>();
	mCommonCBAllocation = mResourceAllocator->AllocateToUploadHeap(&mCommonCB, sizeof(CommonCB), ConstantBufferAlignment);
	
	mLightCB = std::make_unique<LightCB>();
	mLightAllocation = mResourceAllocator->AllocateToUploadHeap(&mLightCB, sizeof(LightCB), ConstantBufferAlignment);

	mTestDeafult = mResourceAllocator->AllocateToDefaultHeap(&mCommonCB, sizeof(CommonCB), ConstantBufferAlignment);
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
	CommonCB currentFrameCB;

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

	mCommonCBAllocation.Copy(&currentFrameCB, sizeof(CommonCB));
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

	mLightAllocation.Copy(&lightData, sizeof(LightCB));
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

	/*DrawGeometryPass();
	DrawLightingPass();
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));*/


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
	D3D12_GPU_VIRTUAL_ADDRESS commonCBAddress = mCommonCBAllocation.GPU;
	mCommandList->SetGraphicsRootConstantBufferView(1, commonCBAddress);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), colorClearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->SetDescriptorHeaps(1, mCBVSRVUAVHeap->GetDescriptorHeap().GetAddressOf());
	mCommandList->SetGraphicsRootDescriptorTable(2, mCBVSRVUAVHeap->GetGpuHandle(0));

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvArray = {CurrentBackBufferExtView()};
	mCommandList->OMSetRenderTargets(rtvArray.size(), rtvArray.data(), true, &mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx));

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : objects)
	{
		object->Draw(mCommandList);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void Demo::DrawGeometryPass()
{
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mPositionMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mNormalMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mAlbedoMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mRoughnessMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mMetalicMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto positionRTV = mRTVHeap->GetCpuHandle(mDescIndex.mPositionDescRtvIdx);
	auto normalRTV = mRTVHeap->GetCpuHandle(mDescIndex.mNormalDescRtvIdx);
	auto albedoRTV = mRTVHeap->GetCpuHandle(mDescIndex.mAlbedoDescRtvIdx);
	auto roughnessRTV = mRTVHeap->GetCpuHandle(mDescIndex.mRoughnessDescRtvIdx);
	auto metalicRTV = mRTVHeap->GetCpuHandle(mDescIndex.mMetalicDescRtvIdx);

	float colorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCommandList->ClearRenderTargetView(positionRTV, colorClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(normalRTV, colorClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(albedoRTV, colorClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(roughnessRTV, colorClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(metalicRTV, colorClearValue, 0, nullptr);

	mCommandList->ClearDepthStencilView(mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->SetPipelineState(mGeometryPass->mPSO.Get());
	mCommandList->SetGraphicsRootSignature(mGeometryPass->mRootSig.Get());

	D3D12_GPU_VIRTUAL_ADDRESS commonCBAddress = mCommonCBAllocation.GPU;
	mCommandList->SetGraphicsRootConstantBufferView(1, commonCBAddress);

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { positionRTV, normalRTV, albedoRTV, roughnessRTV, metalicRTV };
	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(rtvArray.size(), rtvArray.data(),
		true, &mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx));

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : objects)
	{
		object->Draw(mCommandList);
	}
}

void Demo::DrawLightingPass()
{
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mPositionMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mNormalMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mAlbedoMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mRoughnessMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mFrameResource.mMetalicMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float ClearColor[] = { 0.f, 0.f, 0.f, 0.f };
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), ClearColor, 0, nullptr);

	//Set Pipeline & Root signature
	mCommandList->SetPipelineState(mLightingPass->mPSO.Get());
	mCommandList->SetGraphicsRootSignature(mLightingPass->mRootSig.Get());

	D3D12_GPU_VIRTUAL_ADDRESS commonCBAddress = mCommonCBAllocation.GPU;
	mCommandList->SetGraphicsRootConstantBufferView(0, commonCBAddress);

	D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress = mLightAllocation.GPU;
	mCommandList->SetGraphicsRootConstantBufferView(1, lightCBAddress);

	mCommandList->SetDescriptorHeaps(1, mCBVSRVUAVHeap->GetDescriptorHeap().GetAddressOf());

	//Test descriptor heap accessing
	mCommandList->SetGraphicsRootDescriptorTable(2, mCBVSRVUAVHeap->GetGpuHandle(0));

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvArray = { CurrentBackBufferExtView() };
	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(rtvArray.size(), rtvArray.data(), true, nullptr);

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->IASetVertexBuffers(0, 0, nullptr);
	mCommandList->IASetIndexBuffer(nullptr);
	mCommandList->DrawInstanced(3, 1, 0, 0);
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

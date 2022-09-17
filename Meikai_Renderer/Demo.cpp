#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "Camera.h"
#include "BufferFormat.h"
#include "MathHelper.h"
#include "DefaultPass.h"
//#include "Texture.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "CommandList.h"
#include "CommandQueue.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "ResourceStateTracker.h"

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
	auto initList = mDirectCommandQueue->GetCommandList();

	CreateDescriptorHeaps();

	CreateBufferResources();
	CreateBufferDescriptors();

	CreateIBLResources(initList);
	CreateIBLDescriptors();
	BuildModels(initList);

	float aspectRatio = mClientWidth / static_cast<float>(mClientHeight);
	mCamera = std::make_unique<Camera>(aspectRatio);
	BuildFrameResource();
	CreateShader();

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
	mScreenViewport = {0, 0, (float)mClientWidth, (float)mClientHeight,0, 1};

	mDefaultPass = std::make_unique<DefaultPass>(this, mShaders["DefaultForwardVS"], mShaders["DefaultForwardPS"]);
	mGeometryPass = std::make_unique<GeometryPass>(this, mShaders["GeomVS"], mShaders["GeomPS"]);
	mLightingPass = std::make_unique<LightingPass>(this, mShaders["ScreenQuadVS"], mShaders["LightingPS"]);

	auto fenceValue = mDirectCommandQueue->ExecuteCommandList(initList);
	mDirectCommandQueue->WaitForFenceValue(fenceValue);
	m_ContentLoaded = true;
	return true;
}

void Demo::CreateDescriptorHeaps()
{
	mCBVSRVUAVHeap = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mCbvSrvUavDescriptorSize, 128);
	mRTVHeap = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, mRtvDescriptorSize, 128);
	mDSVHeap = std::make_unique<DescriptorHeap>(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, mDsvDescriptorSize, 128);
}

void Demo::CreateBufferResources()
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

	mFrameResource.mRenderTarget = std::make_shared<Texture>(this, colorDesc, &clearAlbedo, TextureUsage::RenderTarget, L"RenderTarget");
	mFrameResource.mPositionMap = std::make_shared<Texture>(this, posDesc, &clearPos, TextureUsage::Position,L"Pos");
	mFrameResource.mNormalMap = std::make_shared<Texture>(this, normalDesc, &clearNormal, TextureUsage::Normalmap, L"Normal");
	mFrameResource.mAlbedoMap = std::make_shared<Texture>(this, colorDesc, &clearAlbedo, TextureUsage::Albedo, L"Albedo");
	mFrameResource.mMetalicMap = std::make_shared<Texture>(this, monoDesc, &clearMetalicRoughnessSSAO, TextureUsage::Metalic, L"Metalic");
	mFrameResource.mRoughnessMap = std::make_shared<Texture>(this, monoDesc, &clearMetalicRoughnessSSAO, TextureUsage::Roughness, L"Roughness");
	mFrameResource.mSsaoMap = std::make_shared<Texture>(this, monoDesc, &clearMetalicRoughnessSSAO, TextureUsage::SSAO, L"SSAO");
	mFrameResource.mDepthStencilBuffer = std::make_shared<Texture>(this, depthDesc, &clearColorDepth, TextureUsage::Depth, L"DepthStencil");

	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mPositionMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mNormalMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mAlbedoMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mMetalicMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mRoughnessMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mSsaoMap->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mDepthStencilBuffer->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mFrameResource.mRenderTarget->GetResource().Get(), D3D12_RESOURCE_STATE_COMMON);
}


void Demo::CreateBufferDescriptors()
{
	mDescIndex.mPositionDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mNormalDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mAlbedoDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mRoughnessDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mMetalicDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mSsaoDescRtvIdx = mRTVHeap->GetNextAvailableIndex();
	mDescIndex.mRenderTargetRtvIdx = mRTVHeap->GetNextAvailableIndex();


	mDescIndex.mPositionDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mNormalDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mAlbedoDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mRoughnessDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mMetalicDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mDepthStencilSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();
	mDescIndex.mSsaoDescSrvIdx = mCBVSRVUAVHeap->GetNextAvailableIndex();

	mDescIndex.mDepthStencilDsvIdx = mDSVHeap->GetNextAvailableIndex();

	
	CreateRtvDescriptor(PositionFormat, mFrameResource.mPositionMap->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mPositionDescRtvIdx));
	CreateRtvDescriptor(NormalFormat, mFrameResource.mNormalMap->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mNormalDescRtvIdx));
	CreateRtvDescriptor(AlbedoFormat, mFrameResource.mAlbedoMap->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mAlbedoDescRtvIdx));
	CreateRtvDescriptor(RoughnessFormat, mFrameResource.mRoughnessMap->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mRoughnessDescRtvIdx));
	CreateRtvDescriptor(MetalicFormat, mFrameResource.mMetalicMap->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mMetalicDescRtvIdx));
	CreateRtvDescriptor(SSAOFormat, mFrameResource.mSsaoMap->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mSsaoDescRtvIdx));
	CreateRtvDescriptor(AlbedoFormat, mFrameResource.mRenderTarget->GetResource(), mRTVHeap->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx));

	CreateSrvDescriptor(PositionFormat, mFrameResource.mPositionMap->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mPositionDescSrvIdx));
	CreateSrvDescriptor(NormalFormat, mFrameResource.mNormalMap->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mNormalDescSrvIdx));
	CreateSrvDescriptor(AlbedoFormat, mFrameResource.mAlbedoMap->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mAlbedoDescSrvIdx));
	CreateSrvDescriptor(RoughnessFormat, mFrameResource.mRoughnessMap->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mRoughnessDescSrvIdx));
	CreateSrvDescriptor(MetalicFormat, mFrameResource.mMetalicMap->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mMetalicDescSrvIdx));
	CreateSrvDescriptor(DepthStencilSRVFormat, mFrameResource.mDepthStencilBuffer->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mDepthStencilSrvIdx));
	CreateSrvDescriptor(SSAOFormat, mFrameResource.mSsaoMap->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mDescIndex.mSsaoDescSrvIdx));

	CreateDsvDescriptor(DepthStencilDSVFormat, mFrameResource.mDepthStencilBuffer->GetResource(), mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx));
}

void Demo::CreateIBLResources(std::shared_ptr<CommandList>& commandList)
{
	mIBLResource.mHDRImage = std::make_shared<Texture>(this);
	commandList->LoadTextureFromFile(*mIBLResource.mHDRImage, L"../textures/Alexs_Apt_2k.hdr", TextureUsage::HDR);
	//mIBLResource.mCubeMap;
	//mIBLResource.mDIffuseCubeMap;
	//mIBLResource.mSpecularCubeMap;
}

void Demo::CreateIBLDescriptors()
{
	mIBLIndex.mHDRImageSrvIndex = mCBVSRVUAVHeap->GetNextAvailableIndex();
	CreateSrvDescriptor(HDRFormat, mIBLResource.mHDRImage->GetResource(), mCBVSRVUAVHeap->GetCpuHandle(mIBLIndex.mHDRImageSrvIndex));
}

void Demo::BuildModels(std::shared_ptr<CommandList>& cmdList)
{
	mModels["Monkey"] = std::make_shared<Model>("../models/Monkey.obj", this, *cmdList);
	mModels["Quad"] = std::make_shared<Model>("../models/Quad.obj", this, *cmdList);
	mModels["Torus"] = std::make_shared<Model>("../models/Torus.obj", this, *cmdList);
	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", this, *cmdList);
	mModels["Skybox"] = std::make_shared<Model>("../models/Skybox.obj", this, *cmdList);

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
	lightData.directLight.Color = XMFLOAT3(10, 10, 10);

	lightData.pointLight[0].Position = XMFLOAT3(2, 0, 0);
	lightData.pointLight[0].Color = XMFLOAT3(10, 0, 0);

	lightData.pointLight[1].Position = XMFLOAT3(0, 2, 0);
	lightData.pointLight[1].Color = XMFLOAT3(0.0, 10, 0.0);

	lightData.pointLight[2].Position = XMFLOAT3(0, 0, 2);
	lightData.pointLight[2].Color = XMFLOAT3(0, 0, 10);

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
	auto drawcmdList = mDirectCommandQueue->GetCommandList();
	DrawDefaultPass(*drawcmdList);
	//DrawGeometryPass(*drawcmdList);
	//DrawLightingPass(*drawcmdList);
	mDirectCommandQueue->ExecuteCommandList(drawcmdList);
	Present(mFrameResource.mRenderTarget);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
}

void Demo::DrawDefaultPass(CommandList& cmdList)
{
	float colorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	cmdList.SetPipelineState(mDefaultPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mDefaultPass->mRootSig.Get());

	//Remove it with bindless later.
	D3D12_GPU_VIRTUAL_ADDRESS commonCBAddress = mCommonCBAllocation.GPU;
	cmdList.SetRootConstant(1, commonCBAddress);

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	cmdList.ClearTexture(mFrameResource.mRenderTarget, mRTVHeap->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx),colorClearValue);
	cmdList.ClearDepthStencilTexture(mFrameResource.mDepthStencilBuffer, mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);

	cmdList.SetDescriptorHeap(mCBVSRVUAVHeap->GetDescriptorHeap());

	cmdList.SetDescriptorTable(2, mCBVSRVUAVHeap->GetGpuHandle(0));

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { mRTVHeap->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx) };
	cmdList.SetRenderTargets(rtvArray, &mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx));

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& object : objects)
	{
		object->Draw(cmdList);
	}
}

void Demo::DrawGeometryPass(CommandList& cmdList)
{
	auto positionRTV = mRTVHeap->GetCpuHandle(mDescIndex.mPositionDescRtvIdx);
	auto normalRTV = mRTVHeap->GetCpuHandle(mDescIndex.mNormalDescRtvIdx);
	auto albedoRTV = mRTVHeap->GetCpuHandle(mDescIndex.mAlbedoDescRtvIdx);
	auto roughnessRTV = mRTVHeap->GetCpuHandle(mDescIndex.mRoughnessDescRtvIdx);
	auto metalicRTV = mRTVHeap->GetCpuHandle(mDescIndex.mMetalicDescRtvIdx);

	auto positionResource = mFrameResource.mPositionMap;
	auto normalResource = mFrameResource.mNormalMap;
	auto albedoResource = mFrameResource.mAlbedoMap;
	auto roughnessResource = mFrameResource.mRoughnessMap;
	auto metalicResource = mFrameResource.mMetalicMap;

	float colorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	cmdList.ClearTexture(positionResource, positionRTV, colorClearValue);
	cmdList.ClearTexture(normalResource, normalRTV, colorClearValue);
	cmdList.ClearTexture(albedoResource, albedoRTV, colorClearValue);
	cmdList.ClearTexture(roughnessResource, roughnessRTV, colorClearValue);
	cmdList.ClearTexture(metalicResource, metalicRTV, colorClearValue);

	cmdList.ClearDepthStencilTexture(mFrameResource.mDepthStencilBuffer, mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);

	cmdList.SetPipelineState(mGeometryPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mGeometryPass->mRootSig.Get());

	D3D12_GPU_VIRTUAL_ADDRESS commonCBAddress = mCommonCBAllocation.GPU;
	cmdList.SetConstantBufferView(1, commonCBAddress);

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { positionRTV, normalRTV, albedoRTV, roughnessRTV, metalicRTV };
	cmdList.SetRenderTargets(rtvArray, &mDSVHeap->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx));

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : objects)
	{
		object->Draw(cmdList);
	}
}

void Demo::DrawLightingPass(CommandList& cmdList)
{
	auto renderTargetResource = mFrameResource.mRenderTarget;

	auto positionResource = mFrameResource.mPositionMap;
	auto normalResource = mFrameResource.mNormalMap;
	auto albedoResource = mFrameResource.mAlbedoMap;
	auto roughnessResource = mFrameResource.mRoughnessMap;
	auto metalicResource = mFrameResource.mMetalicMap;

	auto renderTargetRTV = mRTVHeap->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	cmdList.TransitionBarrier(positionResource->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList.TransitionBarrier(normalResource->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList.TransitionBarrier(albedoResource->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList.TransitionBarrier(roughnessResource->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList.TransitionBarrier(metalicResource->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	// Clear the screen normal map and depth buffer.
	float ClearColor[] = { 0.f, 0.f, 0.f, 0.f };
	cmdList.ClearTexture(renderTargetResource, renderTargetRTV, ClearColor);

	//Set Pipeline & Root signature
	cmdList.SetPipelineState(mLightingPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mLightingPass->mRootSig.Get());

	D3D12_GPU_VIRTUAL_ADDRESS commonCBAddress = mCommonCBAllocation.GPU;
	cmdList.SetConstantBufferView(0, commonCBAddress);

	D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress = mLightAllocation.GPU;
	cmdList.SetConstantBufferView(1, lightCBAddress);

	cmdList.SetDescriptorHeap(mCBVSRVUAVHeap->GetDescriptorHeap());

	cmdList.SetDescriptorTable(2, mCBVSRVUAVHeap->GetGpuHandle(0));

	//// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { renderTargetRTV };
	//// Specify the buffers we are going to render to.
	cmdList.SetRenderTargets(rtvArray, nullptr);
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList.SetEmptyVertexBuffer();
	cmdList.SetEmptyIndexBuffer();
	cmdList.Draw(3);
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

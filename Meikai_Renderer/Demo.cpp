#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "SkeletalObject.h"
#include "Camera.h"
#include "BufferFormat.h"
#include "MathHelper.h"
#include "DefaultPass.h"
#include "DescriptorHeap.h"
#include "Animation.h"
#include "Animator.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "CommandList.h"
#include "CommandQueue.h"

#include "EquiRectToCubemapPass.h"
#include "GeometryPass.h"
#include "SkeletalGeometryPass.h"
#include "LightingPass.h"
#include "JointDebugPass.h"
#include "BoneDebugPass.h"
#include "SkyboxPass.h"
#include "Texture.h"

#include "ResourceStateTracker.h"
#include "SkeletalModel.h"

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
	CreateIBLResources(initList);
	BuildModels(initList);
	LoadAnimations();
	BuildObjects();

	float aspectRatio = mClientWidth / static_cast<float>(mClientHeight);
	mCamera = std::make_unique<Camera>(aspectRatio);
	BuildFrameResource();
	CreateShader();

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
	mScreenViewport = {0, 0, (float)mClientWidth, (float)mClientHeight,0, 1};
	mDefaultPass = std::make_unique<DefaultPass>(this, mShaders["DefaultForwardVS"], mShaders["DefaultForwardPS"]);
	mGeometryPass = std::make_unique<GeometryPass>(this, mShaders["GeomVS"], mShaders["GeomPS"]);
	mLightingPass = std::make_unique<LightingPass>(this, mShaders["ScreenQuadVS"], mShaders["LightingPS"], 
		mIBLResource.mDiffuseMap->mSRVDescIDX.value(), mIBLResource.mSpecularMap->mSRVDescIDX.value());
	mJointDebugPass = std::make_unique<JointDebugPass>(this, mShaders["DebugJointVS"], mShaders["DebugJointPS"]);
	mBoneDebugPass = std::make_unique<BoneDebugPass>(this, mShaders["DebugJointVS"], mShaders["DebugJointPS"]);
	mSkyboxPass = std::make_unique<SkyboxPass>(this, mShaders["SkyboxVS"], mShaders["SkyboxPS"], mIBLResource.mSkyboxCubeMap->mSRVDescIDX.value());
	mSkeletalGeometryPass = std::make_unique<SkeletalGeometryPass>(this, mShaders["SkeletalGeomVS"], mShaders["SkeletalGeomPS"]);

	mEquiRectToCubemapPass = std::make_unique<EquiRectToCubemapPass>(this, mShaders["EquiRectToCubemapCS"], 
		mIBLResource.mHDRImage->mSRVDescIDX.value(), mIBLResource.mSkyboxCubeMap->mUAVDescIDX.value());

	auto fenceValue = mDirectCommandQueue->ExecuteCommandList(initList);
	mDirectCommandQueue->WaitForFenceValue(fenceValue);
	m_ContentLoaded = true;

	PreCompute();

	return true;
}

void Demo::Update(const GameTimer& gt)
{
	mCamera->Update(gt);
	UpdatePassCB(gt);
	UpdateLightCB(gt);
	for(auto& skeletalObject : mSkeletalObjects)
	{
		skeletalObject->Update(gt.DeltaTime());
	}
}

void Demo::Draw(const GameTimer& gt)
{
	auto drawcmdList = mDirectCommandQueue->GetCommandList();
	//DrawDefaultPass(*drawcmdList);
	DrawGeometryPasses(*drawcmdList);
	DrawLightingPass(*drawcmdList);
	DrawSkyboxPass(*drawcmdList);
	DrawJointDebug(*drawcmdList);
	DrawBoneDebug(*drawcmdList);
	mDirectCommandQueue->ExecuteCommandList(drawcmdList);
	Present(mFrameResource.mRenderTarget);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
}

void Demo::PreCompute()
{
	auto initList = mDirectCommandQueue->GetCommandList();
	DispatchEquiRectToCubemap(*initList);
	
	auto fenceValue = mDirectCommandQueue->ExecuteCommandList(initList);
	mDirectCommandQueue->WaitForFenceValue(fenceValue);
}

void Demo::BuildModels(std::shared_ptr<CommandList>& cmdList)
{
	mModels["Skybox"] = std::make_shared<Model>("../models/Skybox.obj", this, *cmdList);
	mSkeletalModels["X_Bot"] = std::make_shared<SkeletalModel>("../models/X_Bot.dae", this, *cmdList);
	mSkeletalModels["Y_Bot"] = std::make_shared<SkeletalModel>("../models/Y_Bot.dae", this, *cmdList);
}

void Demo::LoadAnimations()
{
	mAnimations["walking"] = std::make_shared<Animation>("../animations/Walking.dae", mSkeletalModels["Y_Bot"]);
	mAnimations["dancing"] = std::make_shared<Animation>("../animations/Dancing.dae", mSkeletalModels["X_Bot"]);
}

void Demo::BuildObjects()
{
	mSkeletalObjects.push_back(std::make_unique<SkeletalObject>(this, mSkeletalModels["Y_Bot"], mAnimations["walking"], XMFLOAT3(1.f, -1.f, 0.f)));
	mSkeletalObjects.push_back(std::make_unique<SkeletalObject>(this, mSkeletalModels["X_Bot"], mAnimations["dancing"], XMFLOAT3(-1.f, -1.f, 0.f)));

	mSkybox = std::make_unique<Object>(mModels["Skybox"], XMFLOAT3(0.f, 0.f, 0.f));
}

void Demo::BuildFrameResource()
{
	const size_t ConstantBufferAlignment = 256;

	mCommonCB = std::make_unique<CommonCB>();
	mLightCB = std::make_unique<LightCB>();
}

void Demo::CreateIBLResources(std::shared_ptr<CommandList>& commandList)
{
	mIBLResource.mHDRImage = std::make_shared<Texture>(this);
	commandList->LoadTextureFromFile(*mIBLResource.mHDRImage, L"../textures/Tropical_Beach_3k.hdr", D3D12_SRV_DIMENSION_TEXTURE2D);
	mIBLResource.mDiffuseMap = std::make_shared<Texture>(this);
	commandList->LoadTextureFromFile(*mIBLResource.mDiffuseMap, L"../textures/Tropical_Beach_3k.irr.hdr", D3D12_SRV_DIMENSION_TEXTURE2D);
	mIBLResource.mSpecularMap = std::make_shared<Texture>(this);

	commandList->LoadTextureFromFile(*mIBLResource.mSpecularMap, L"../textures/Tropical_Beach_3k.irr.hdr", D3D12_SRV_DIMENSION_TEXTURE2D);

	auto cubemapDesc = mIBLResource.mHDRImage->GetD3D12ResourceDesc();
	cubemapDesc.Format = AlbedoFormat;
	cubemapDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	cubemapDesc.Width = cubemapDesc.Height = 1024;
	cubemapDesc.DepthOrArraySize = 6;
	cubemapDesc.MipLevels = 0;
	mIBLResource.mSkyboxCubeMap = std::make_shared<Texture>(this, cubemapDesc, nullptr, D3D12_SRV_DIMENSION_TEXTURECUBE, D3D12_UAV_DIMENSION_TEXTURE2DARRAY, L"SkyboxCubemap");
	cubemapDesc.Format = HDRFormat;

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
	mShaders["DebugJointVS"] = DxUtil::CompileShader(L"../shaders/DebugJoint.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["DebugJointPS"] = DxUtil::CompileShader(L"../shaders/DebugJoint.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["SkeletalGeomVS"] = DxUtil::CompileShader(L"../shaders/SkeletalGeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkeletalGeomPS"] = DxUtil::CompileShader(L"../shaders/SkeletalGeometryPass.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["SkyboxVS"] = DxUtil::CompileShader(L"../shaders/SkyboxPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyboxPS"] = DxUtil::CompileShader(L"../shaders/SkyboxPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["EquiRectToCubemapCS"] = DxUtil::CompileShader(L"../shaders/EquiRectToCubemap.hlsl", nullptr, "EquiRectToCubemapCS", "cs_5_1");
	mShaders["CalcIBLDiffuseCS"] = DxUtil::CompileShader(L"../shaders/CalcIBLDiffuse.hlsl", nullptr, "CalcIBLDiffuseCS", "cs_5_1");
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

	*mCommonCB = currentFrameCB;
}

void Demo::UpdateLightCB(const GameTimer& gt)
{
	LightCB currentFramelightData;

	currentFramelightData.directLight.Direction = XMFLOAT3(-0.5f, 0.5f, 0.5f);
	currentFramelightData.directLight.Color = XMFLOAT3(10, 10, 10);

	currentFramelightData.pointLight[0].Position = XMFLOAT3(1.5, 1.5, 1.5);
	currentFramelightData.pointLight[0].Color = XMFLOAT3(70, 70, 70);

	currentFramelightData.pointLight[1].Position = XMFLOAT3(-1.5, -1.5, -1.5);
	currentFramelightData.pointLight[1].Color = XMFLOAT3(70, 10, 70);

	currentFramelightData.pointLight[2].Position = XMFLOAT3(0, 0, 10);
	currentFramelightData.pointLight[2].Color = XMFLOAT3(0, 0, 10);

	*mLightCB = currentFramelightData;
}

void Demo::DrawDefaultPass(CommandList& cmdList)
{
	float colorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	cmdList.SetPipelineState(mDefaultPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mDefaultPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);
	auto dsvHeapCPUHandle = mDescriptorHeaps[DSV]->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx);

	auto rtvHeap = mDescriptorHeaps[RTV];
	cmdList.ClearTexture(mFrameResource.mRenderTarget, rtvHeapCPUHandle,colorClearValue);
	cmdList.ClearDepthStencilTexture(mFrameResource.mDepthStencilBuffer, dsvHeapCPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { rtvHeapCPUHandle };
	cmdList.SetRenderTargets(rtvArray, &dsvHeapCPUHandle);

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& object : mObjects)
	{
		object->Draw(cmdList);
	}
}

void Demo::DrawGeometryPasses(CommandList& cmdList)
{
	auto rtvHeap = mDescriptorHeaps[RTV];
	auto dsvHeapCPUHandle = mDescriptorHeaps[DSV]->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx);

	auto positionRTV = rtvHeap->GetCpuHandle(mDescIndex.mPositionDescRtvIdx);
	auto normalRTV = rtvHeap->GetCpuHandle(mDescIndex.mNormalDescRtvIdx);
	auto albedoRTV = rtvHeap->GetCpuHandle(mDescIndex.mAlbedoDescRtvIdx);
	auto roughnessRTV = rtvHeap->GetCpuHandle(mDescIndex.mRoughnessDescRtvIdx);
	auto metalicRTV = rtvHeap->GetCpuHandle(mDescIndex.mMetalicDescRtvIdx);

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

	cmdList.ClearDepthStencilTexture(mFrameResource.mDepthStencilBuffer, dsvHeapCPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);

	cmdList.SetPipelineState(mGeometryPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mGeometryPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { positionRTV, normalRTV, albedoRTV, roughnessRTV, metalicRTV };
	cmdList.SetRenderTargets(rtvArray, &dsvHeapCPUHandle);

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : mObjects)
	{
		object->Draw(cmdList);
	}

	cmdList.SetPipelineState(mSkeletalGeometryPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mSkeletalGeometryPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	for (const auto& skeletalObject : mSkeletalObjects)
	{
		skeletalObject->Draw(cmdList);
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

	auto rtvHeap = mApp->GetDescriptorHeap(RTV);
	auto renderTargetRTV = rtvHeap->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	auto srvTex2DHeap = mApp->GetDescriptorHeap(SRV_2D);

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

	cmdList.SetGraphicsDynamicConstantBuffer(0, sizeof(CommonCB), mCommonCB.get());
	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(LightCB), mLightCB.get());

	//Set tables for geometry textures
	cmdList.SetDescriptorHeap(srvTex2DHeap->GetDescriptorHeap());
	cmdList.SetGraphicsDescriptorTable(2, srvTex2DHeap->GetGpuHandle(0));

	//Set descriptor indices
	cmdList.SetGraphics32BitConstants(3, mLightingPass->mLightDescIndices.TexNum + 1, &(mLightingPass->mLightDescIndices));

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

void Demo::DrawSkyboxPass(CommandList& cmdList)
{
	auto srvCubeHeap = mDescriptorHeaps[SRV_CUBE];
	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);
	auto dsvHeapCPUHandle = mDescriptorHeaps[DSV]->GetCpuHandle(mDescIndex.mDepthStencilDsvIdx);

	cmdList.SetPipelineState(mSkyboxPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mSkyboxPass->mRootSig.Get());

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	cmdList.SetGraphicsDynamicConstantBuffer(0, sizeof(CommonCB), mCommonCB.get());
	cmdList.SetDescriptorHeap(srvCubeHeap->GetDescriptorHeap());
	cmdList.SetGraphicsDescriptorTable(1, srvCubeHeap->GetGpuHandle(0));
	cmdList.SetGraphics32BitConstants(2, mSkyboxPass->mSkyboxDescIndices.TexNum + 1, &(mSkyboxPass->mSkyboxDescIndices));

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { rtvHeapCPUHandle };
	cmdList.SetRenderTargets(rtvArray, &dsvHeapCPUHandle);

	mSkybox->DrawWithoutWorld(cmdList);
}

void Demo::DrawJointDebug(CommandList& cmdList)
{
	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	cmdList.SetPipelineState(mJointDebugPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mJointDebugPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { rtvHeapCPUHandle };
	cmdList.SetRenderTargets(rtvArray, nullptr);
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	for (auto& object : mSkeletalObjects)
	{
		object->DrawJoint(cmdList);
	}
}

void Demo::DrawBoneDebug(CommandList& cmdList)
{
	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	cmdList.SetPipelineState(mBoneDebugPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mBoneDebugPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	cmdList.SetViewport(mScreenViewport);
	cmdList.SetScissorRect(mScissorRect);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { rtvHeapCPUHandle };
	cmdList.SetRenderTargets(rtvArray, nullptr);
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); 

	for (auto& object : mSkeletalObjects)
	{
		object->DrawBone(cmdList);
	}
}

void Demo::DispatchEquiRectToCubemap(CommandList& cmdList)
{
	auto device = mApp->GetDevice();

	auto cubemapResource = mIBLResource.mSkyboxCubeMap->GetResource();
	CD3DX12_RESOURCE_DESC cubemapDesc(cubemapResource->GetDesc());

	cmdList.SetPipelineState(mEquiRectToCubemapPass->mPSO.Get());
	cmdList.SetComputeRootSignature(mEquiRectToCubemapPass->mRootSig.Get());

	EquiRectToCubemapCB equiRectToCubemapCB;
	equiRectToCubemapCB.CubemapSize = std::max<uint32_t>(cubemapDesc.Width, cubemapDesc.Height);

	auto srvHeap = mApp->GetDescriptorHeap(SRV_2D);
	auto uavHeap = mApp->GetDescriptorHeap(UAV_2D_ARRAY);

	cmdList.SetCompute32BitConstants(0, equiRectToCubemapCB);
	cmdList.SetDescriptorHeap(srvHeap->GetDescriptorHeap());
	cmdList.SetComputeDescriptorTable(1, srvHeap->GetGpuHandle(0));

	cmdList.SetDescriptorHeap(uavHeap->GetDescriptorHeap());
	cmdList.SetComputeDescriptorTable(2, uavHeap->GetGpuHandle(0));

	cmdList.SetCompute32BitConstants(3, mEquiRectToCubemapPass->mEquiRectDescIndices.TexNum + 1, &(mEquiRectToCubemapPass->mEquiRectDescIndices));

	cmdList.Dispatch(MathHelper::DivideByMultiple(equiRectToCubemapCB.CubemapSize, 16), MathHelper::DivideByMultiple(equiRectToCubemapCB.CubemapSize, 16), 6);
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

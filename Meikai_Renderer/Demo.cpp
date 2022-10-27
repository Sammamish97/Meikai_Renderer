#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "SkeletalObject.h"
#include "Camera.h"
#include "BufferFormat.h"
#include "MathHelper.h"
#include "DescriptorHeap.h"

#include "PathGenerator.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "CommandList.h"
#include "CommandQueue.h"

#include "EquiRectToCubemapPass.h"
#include "GeometryPass.h"
#include "SkeletalGeometryPass.h"
#include "LightingPass.h"
#include "DebugMeshPass.h"
#include "DebugLinePass.h"
#include "SkyboxPass.h"
#include "ShadowPass.h"
#include "SsaoPass.h"
#include "BlurPass.h"
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
	mPathGenerator = std::make_unique<PathGenerator>(mModels["Sphere"]);

	BuildFrameResource();
	CreateShaderFromHLSL();

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
	mScreenViewport = {0, 0, (float)mClientWidth, (float)mClientHeight,0, 1};
	mGeometryPass = std::make_unique<GeometryPass>(this, mShaders["GeomVS"], mShaders["GeomPS"]);
	mLightingPass = std::make_unique<LightingPass>(this, mShaders["ScreenQuadVS"], mShaders["LightingPS"], 
		mIBLResource.mDiffuseMap->mSRVDescIDX.value(), mIBLResource.mHDRImage->mSRVDescIDX.value());
	mJointDebugPass = std::make_unique<DebugMeshPass>(this, mShaders["DebugJointVS"], mShaders["DebugJointPS"]);
	mBoneDebugPass = std::make_unique<DebugLinePass>(this, mShaders["DebugJointVS"], mShaders["DebugJointPS"]);
	mSkyboxPass = std::make_unique<SkyboxPass>(this, mShaders["SkyboxVS"], mShaders["SkyboxPS"], mIBLResource.mSkyboxCubeMap->mSRVDescIDX.value());
	mSkeletalGeometryPass = std::make_unique<SkeletalGeometryPass>(this, mShaders["SkeletalGeomVS"], mShaders["SkeletalGeomPS"]);
	mShadowPass = std::make_unique<ShadowPass>(this, mShaders["ShadowVS"], mShaders["ShadowPS"]);
	mSsaoPass = std::make_unique<SsaoPass>(this, mShaders["ScreenQuadVS"], mShaders["SsaoPS"]);
	mBlurHPass = std::make_unique<BlurPass>(this, mShaders["HBlurCS"], true);
	mBlurVPass = std::make_unique<BlurPass>(this, mShaders["VBlurCS"], false);

	mEquiRectToCubemapPass = std::make_unique<EquiRectToCubemapPass>(this, mShaders["EquiRectToCubemapCS"], 
		mIBLResource.mHDRImage->mSRVDescIDX.value(), mIBLResource.mSkyboxCubeMap->mUAVDescIDX.value());

	auto fenceValue = mDirectCommandQueue->ExecuteCommandList(initList);
	mDirectCommandQueue->WaitForFenceValue(fenceValue);
	m_ContentLoaded = true;

	PreCompute();
	InitImgui();

	return true;
}

void Demo::InitImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(MainWnd());

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (mdxDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
	{
		assert("Failed to create IMGUI SRV heap");
	}

	ImGui_ImplDX12_Init(mdxDevice.Get(), 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap.Get(),
		g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void Demo::StartImGuiFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Demo::ClearImGui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Demo::Update(const GameTimer& gt)
{
	StartImGuiFrame();
	mCamera->Update(gt);
	UpdatePassCB(gt);
	UpdateLightCB(gt);
	float tick = mPathGenerator->Update(gt, mMoveTestSkeletal->GetTicksPerSec(), mMoveTestSkeletal->GetDuration(), mMoveTestSkeletal->GetDistacnePerDuration());
  	mMoveTestSkeletal->SetPosition(mPathGenerator->GetPosition());
	mMoveTestSkeletal->SetDirection(mPathGenerator->GetDirection());
	for(auto& skeletalObject : mSkeletalObjects)
	{
		skeletalObject->Update(gt.DeltaTime() * skeletalObject->GetTicksPerSec());
	}
	mMoveTestSkeletal->Update(tick);
}

void Demo::Draw(const GameTimer& gt)
{
	auto drawcmdList = mDirectCommandQueue->GetCommandList();
	DrawShadowPass(*drawcmdList);
	DrawGeometryPasses(*drawcmdList);
	DrawSsaoPass(*drawcmdList);
	DispatchBluring(*drawcmdList);
	DrawLightingPass(*drawcmdList);
	DrawSkyboxPass(*drawcmdList);
	DrawBoneDebug(*drawcmdList);
	DrawPathDebug(*drawcmdList);
	DrawMeshDebug(*drawcmdList);
	DrawGUI(*drawcmdList);
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
	mModels["Sphere"] = std::make_shared<Model>("../models/Sphere.obj", this, *cmdList);
	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", this, *cmdList);
	/*mModels["Cube"] = std::make_shared<Model>("../models/Cube.obj", this, *cmdList);
	mModels["Torus"] = std::make_shared<Model>("../models/Torus.obj", this, *cmdList);
	mModels["Monkey"] = std::make_shared<Model>("../models/Monkey.obj", this, *cmdList);
	mModels["bunny"] = std::make_shared<Model>("../models/bunny.obj", this, *cmdList);
	mModels["dragon"] = std::make_shared<Model>("../models/dragon.obj", this, *cmdList);*/

	//mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", this, *cmdList);
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
	//mSkeletalObjects.push_back(std::make_unique<SkeletalObject>(this, mSkeletalModels["X_Bot"], mAnimations["dancing"], XMFLOAT3(1.f, 0.f, 0.f)));
	mObjects.push_back(std::make_unique<Object>(mModels["Plane"], XMFLOAT3(0, 0, 0), XMFLOAT3(0.1f, 0.1, 0.1f)));
	/*mObjects.push_back(std::make_unique<Object>(mModels["Cube"], XMFLOAT3(-2, 0, 2), XMFLOAT3(1, 1, 1)));
	mObjects.push_back(std::make_unique<Object>(mModels["bunny"], XMFLOAT3(2, -1, 2), XMFLOAT3(1, 1, 1)));
	mObjects.push_back(std::make_unique<Object>(mModels["dragon"], XMFLOAT3(-2, 0, -2), XMFLOAT3(5 ,5, 5)));
	mObjects.push_back(std::make_unique<Object>(mModels["Sphere"], XMFLOAT3(2, 0, -2), XMFLOAT3(1, 1, 1)));*/

	mSkybox = std::make_unique<Object>(mModels["Skybox"], XMFLOAT3(0.f, 0.f, 0.f));
	mMoveTestSkeletal = std::make_unique<SkeletalObject>(this, mSkeletalModels["Y_Bot"], mAnimations["walking"], XMFLOAT3(0.f, 0.f, 0.f));
}

void Demo::BuildFrameResource()
{
	mCommonCB = std::make_unique<CommonCB>();
	mLightCB = std::make_unique<LightCB>();
	mRandomSampleCB = std::make_unique<RandomSampleCB>(mIBLResource.mHDRImage->GetD3D12ResourceDesc().Width, mIBLResource.mHDRImage->GetD3D12ResourceDesc().Height);
}

void Demo::CreateIBLResources(std::shared_ptr<CommandList>& commandList)
{
	mIBLResource.mHDRImage = std::make_shared<Texture>(this);
	commandList->LoadTextureFromFile(*mIBLResource.mHDRImage, L"../textures/Alexs_Apt_2k.hdr", D3D12_SRV_DIMENSION_TEXTURE2D);
	mIBLResource.mDiffuseMap = std::make_shared<Texture>(this);
	commandList->LoadTextureFromFile(*mIBLResource.mDiffuseMap, L"../textures/Alexs_Apt_2k.irr.hdr", D3D12_SRV_DIMENSION_TEXTURE2D);

	auto cubemapDesc = mIBLResource.mHDRImage->GetD3D12ResourceDesc();
	cubemapDesc.Format = AlbedoFormat;
	cubemapDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	cubemapDesc.Width = cubemapDesc.Height = 1024;
	cubemapDesc.DepthOrArraySize = 6;
	cubemapDesc.MipLevels = 0;
	mIBLResource.mSkyboxCubeMap = std::make_shared<Texture>(this, cubemapDesc, nullptr, D3D12_SRV_DIMENSION_TEXTURECUBE, D3D12_UAV_DIMENSION_TEXTURE2DARRAY, L"SkyboxCubemap");
	cubemapDesc.Format = HDRFormat;

}

void Demo::CreateShaderFromHLSL()
{
	mShaders["GeomVS"] = DxUtil::CompileShader(L"../shaders/GeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["GeomPS"] = DxUtil::CompileShader(L"../shaders/GeometryPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ScreenQuadVS"] = DxUtil::CompileShader(L"../shaders/ScreenQuad.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["LightingPS"] = DxUtil::CompileShader(L"../shaders/LightingPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SsaoPS"] = DxUtil::CompileShader(L"../shaders/SsaoPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["DebugJointVS"] = DxUtil::CompileShader(L"../shaders/DebugJoint.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["DebugJointPS"] = DxUtil::CompileShader(L"../shaders/DebugJoint.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SkeletalGeomVS"] = DxUtil::CompileShader(L"../shaders/SkeletalGeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkeletalGeomPS"] = DxUtil::CompileShader(L"../shaders/SkeletalGeometryPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["SkyboxVS"] = DxUtil::CompileShader(L"../shaders/SkyboxPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyboxPS"] = DxUtil::CompileShader(L"../shaders/SkyboxPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ShadowVS"] = DxUtil::CompileShader(L"../shaders/ShadowPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ShadowPS"] = DxUtil::CompileShader(L"../shaders/ShadowPass.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["HBlurCS"] = DxUtil::CompileShader(L"../shaders/Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_1");
	mShaders["VBlurCS"] = DxUtil::CompileShader(L"../shaders/Blur.hlsl", nullptr, "VertBlurCS", "cs_5_1");
	mShaders["EquiRectToCubemapCS"] = DxUtil::CompileShader(L"../shaders/EquiRectToCubemap.hlsl", nullptr, "EquiRectToCubemapCS", "cs_5_1");
}

void Demo::CreateShaderFromCSO()
{
	mShaders["GeomVS"] = DxUtil::LoadCSO(L"../shaders/GeomVS.cso");
	mShaders["GeomPS"] = DxUtil::LoadCSO(L"../shaders/GeomPS.cso");

	mShaders["ScreenQuadVS"] = DxUtil::LoadCSO(L"../shaders/ScreenQuadVS.cso");
	mShaders["LightingPS"] = DxUtil::LoadCSO(L"../shaders/LightingPS.cso");

	mShaders["SsaoPS"] = DxUtil::LoadCSO(L"../shaders/SsaoPS.cso");

	mShaders["DefaultForwardVS"] = DxUtil::LoadCSO(L"../shaders/DefaultForwardVS.cso");
	mShaders["DefaultForwardPS"] = DxUtil::LoadCSO(L"../shaders/DefaultForwardPS.cso");

	mShaders["DebugJointVS"] = DxUtil::LoadCSO(L"../shaders/DebugJointVS.cso");
	mShaders["DebugJointPS"] = DxUtil::LoadCSO(L"../shaders/DebugJointPS.cso");

	mShaders["SkeletalGeomVS"] = DxUtil::LoadCSO(L"../shaders/SkeletalGeomVS.cso");
	mShaders["SkeletalGeomPS"] = DxUtil::LoadCSO(L"../shaders/SkeletalGeomPS.cso");

	mShaders["SkyboxVS"] = DxUtil::LoadCSO(L"../shaders/SkyboxVS.cso");
	mShaders["SkyboxPS"] = DxUtil::LoadCSO(L"../shaders/SkyboxPS.cso");

	mShaders["ShadowVS"] = DxUtil::LoadCSO(L"../shaders/ShadowVS.cso");
	mShaders["ShadowPS"] = DxUtil::LoadCSO(L"../shaders/ShadowPS.cso");

	mShaders["HBlurCS"] = DxUtil::LoadCSO(L"../shaders/HBlurCS.cso");
	mShaders["VBlurCS"] = DxUtil::LoadCSO(L"../shaders/VBlurCS.cso");

	mShaders["EquiRectToCubemapCS"] = DxUtil::LoadCSO(L"../shaders/EquiRectToCubemapCS.cso");
}

void Demo::UpdatePassCB(const GameTimer& gt)
{
	static float testRoughness = 0;
	static float testMetalic = 0;

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

	//Temporaily, Total time is Roughness and Delta time is metalic
	//currentFrameCB.TotalTime = gt.TotalTime();
	//currentFrameCB.DeltaTime = gt.DeltaTime();


	ImGui::Begin("PBR_PARAMS");                          // Create a window called "Hello, world!" and append into it.
	ImGui::SliderFloat("Roughness", &testRoughness, 0.0f, 1.f);            // Edit 1 float using a slider from 0.0f to 1.0f
	ImGui::SliderFloat("Metalic", &testMetalic, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
	ImGui::End();

	currentFrameCB.TotalTime = testRoughness;
	currentFrameCB.DeltaTime = testMetalic;

	*mCommonCB = currentFrameCB;
}

void Demo::UpdateLightCB(const GameTimer& gt)
{
	LightCB currentFramelightData;

	currentFramelightData.directLight.Direction = XMFLOAT3(-0.5f, 0.5f, 0.5f);
	currentFramelightData.directLight.Color = XMFLOAT3(10, 10, 10);

	currentFramelightData.pointLight[0].Position = XMFLOAT3(10, 10, 10);
	currentFramelightData.pointLight[0].Color = XMFLOAT3(0, 0, 0);

	currentFramelightData.pointLight[1].Position = XMFLOAT3(-15, -15, -15);
	currentFramelightData.pointLight[1].Color = XMFLOAT3(0, 0, 0);

	currentFramelightData.pointLight[2].Position = XMFLOAT3(0, 0, 10);
	currentFramelightData.pointLight[2].Color = XMFLOAT3(0, 0, 0);

	*mLightCB = currentFramelightData;
}

XMFLOAT4X4 Demo::BuildShadowMatrix(bool isShadowPass)
{
	float aspectRatio = mClientWidth / static_cast<float>(mClientHeight);
	auto lightPosition = XMLoadFloat3(&mLightCB->pointLight[0].Position);
	auto lightDir = XMVectorNegate(lightPosition);

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(lightPosition, lightDir, up);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspectRatio, 1.0f, 1000.0f);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S;
	S = viewProj * T;
	if(isShadowPass)
	{
		S = viewProj;
	}

	XMFLOAT4X4 result;
	XMStoreFloat4x4(&result, XMMatrixTranspose(S));
	return result;
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
	mMoveTestSkeletal->Draw(cmdList);
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
	cmdList.SetGraphicsDynamicConstantBuffer(4, sizeof(RandomSampleCB), mRandomSampleCB.get());

	//Set tables for geometry textures
	cmdList.SetDescriptorHeap(srvTex2DHeap->GetDescriptorHeap());
	cmdList.SetGraphicsDescriptorTable(2, srvTex2DHeap->GetGpuHandle(0));

	//Set descriptor indices
	cmdList.SetGraphics32BitConstants(3, mLightingPass->mLightDescIndices.TexNum + 1, &(mLightingPass->mLightDescIndices));
	cmdList.SetGraphics32BitConstants(5, BuildShadowMatrix(false));

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

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST); 

	for (auto& object : mSkeletalObjects)
	{
		object->DrawBone(cmdList);
	}
}

void Demo::DrawPathDebug(CommandList& cmdList)
{
	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	cmdList.SetPipelineState(mBoneDebugPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mBoneDebugPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	cmdList.SetGraphics32BitConstants(0, XMMatrixIdentity());
	mPathGenerator->DrawPaths(cmdList);
}

void Demo::DrawMeshDebug(CommandList& cmdList)
{
	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	cmdList.SetPipelineState(mJointDebugPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mJointDebugPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mPathGenerator->DrawControlPoints(cmdList);
}

void Demo::DrawShadowPass(CommandList& cmdList)
{
	auto dsvHeapCPUHandle = mDescriptorHeaps[DSV]->GetCpuHandle(mDescIndex.mShadowDepthDsvIdx);

	cmdList.ClearDepthStencilTexture(mFrameResource.mShadowDepthBuffer, dsvHeapCPUHandle, D3D12_CLEAR_FLAG_DEPTH);

	cmdList.SetPipelineState(mShadowPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mShadowPass->mRootSig.Get());

	cmdList.SetRenderTargets(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>(), &dsvHeapCPUHandle);

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList.SetGraphics32BitConstants(1, BuildShadowMatrix(true));
	for (const auto& object : mObjects)
	{
		object->Draw(cmdList);
	}
	//TODO: Add pass and drawing pass for animated object.
}

void Demo::DrawSsaoPass(CommandList& cmdList)
{
	auto srvTex2DHeap = mApp->GetDescriptorHeap(SRV_2D);

	auto rtvHeap = mApp->GetDescriptorHeap(RTV);
	auto ssaoRTV = rtvHeap->GetCpuHandle(mDescIndex.mSsaoDescRtvIdx);

	auto ssaoResource = mFrameResource.mSsaoMap;

	float colorClearValue[] = { 0.f, 0.f, 0.f, 0.f };

	cmdList.ClearTexture(ssaoResource, ssaoRTV, colorClearValue);

	cmdList.SetPipelineState(mSsaoPass->mPSO.Get());
	cmdList.SetGraphicsRootSignature(mSsaoPass->mRootSig.Get());

	cmdList.SetGraphicsDynamicConstantBuffer(0, sizeof(CommonCB), mCommonCB.get());

	//Set tables for geometry textures
	cmdList.SetDescriptorHeap(srvTex2DHeap->GetDescriptorHeap());
	cmdList.SetGraphicsDescriptorTable(1, srvTex2DHeap->GetGpuHandle(0));

	//Set descriptor indices
	cmdList.SetGraphics32BitConstants(2, mSsaoPass->mSsaoIndices.TexNum + 1, &(mSsaoPass->mSsaoIndices));

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvArray = { ssaoRTV };
	//// Specify the buffers we are going to render to.
	cmdList.SetRenderTargets(rtvArray, nullptr);
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList.SetEmptyVertexBuffer();
	cmdList.SetEmptyIndexBuffer();
	cmdList.Draw(3);
}

void Demo::DrawGUI(CommandList& cmdList)
{
	auto rtvHeapCPUHandle = mDescriptorHeaps[RTV]->GetCpuHandle(mDescIndex.mRenderTargetRtvIdx);

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList.SetDescriptorHeap(g_pd3dSrvDescHeap);
	
	ImGui::Render();

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.GetList().Get());
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

void Demo::DispatchBluring(CommandList& cmdList)
{
	//0: H
	//1: V
	auto srvHeap = mApp->GetDescriptorHeap(SRV_2D);
	auto uavHeap = mApp->GetDescriptorHeap(UAV_2D);

	auto ssaoTexWidth = mFrameResource.mSsaoMap->GetD3D12ResourceDesc().Width;
	auto ssaoTexHeight = mFrameResource.mSsaoMap->GetD3D12ResourceDesc().Height;

	auto weights = mBlurHPass->CalcGaussWeights(0.5f);
	int blurRadius = (int)weights.size() / 2;

	cmdList.SetComputeRootSignature(mBlurHPass->mRootSig);

	cmdList.GetList()->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
	cmdList.GetList()->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

	cmdList.CopyResource(mFrameResource.mBlurBufferH->GetResource(), mFrameResource.mSsaoMap->GetResource());
	cmdList.FlushResourceBarriers();

	cmdList.TransitionBarrier(mFrameResource.mBlurBufferH->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);//이것만 transition되고 
	cmdList.TransitionBarrier(mFrameResource.mBlurBufferV->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);//이것은 transition이 안됨.
	cmdList.FlushResourceBarriers();

	cmdList.SetDescriptorHeap(srvHeap->GetDescriptorHeap());
	cmdList.SetComputeDescriptorTable(1, srvHeap->GetGpuHandle(0));

	cmdList.SetDescriptorHeap(uavHeap->GetDescriptorHeap());
	cmdList.SetComputeDescriptorTable(2, uavHeap->GetGpuHandle(0));

	for (int i = 0; i < blurCount; ++i)
	{
		//
		// Horizontal Blur pass.
		//

		cmdList.SetPipelineState(mBlurHPass->mPSO);

		cmdList.SetCompute32BitConstants(3, mBlurHPass->mBlurDescIndices.TexNum + 1, &(mBlurHPass->mBlurDescIndices));//Desc Indices

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		UINT numGroupsX = (UINT)ceilf(ssaoTexWidth / 256.0f);
		cmdList.Dispatch(numGroupsX, ssaoTexHeight, 1);

		cmdList.TransitionBarrier(mFrameResource.mBlurBufferH->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cmdList.TransitionBarrier(mFrameResource.mBlurBufferV->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		cmdList.FlushResourceBarriers();

		//
		// Vertical Blur pass.
		//

		cmdList.SetPipelineState(mBlurVPass->mPSO);

		cmdList.SetCompute32BitConstants(3, mBlurVPass->mBlurDescIndices.TexNum + 1, &(mBlurVPass->mBlurDescIndices));//Desc Indices

		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		UINT numGroupsY = (UINT)ceilf(ssaoTexHeight / 256.0f);
		cmdList.Dispatch(ssaoTexWidth, numGroupsY, 1);

		cmdList.TransitionBarrier(mFrameResource.mBlurBufferH->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		cmdList.TransitionBarrier(mFrameResource.mBlurBufferV->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cmdList.FlushResourceBarriers();
	}

	cmdList.CopyResource(mFrameResource.mSsaoMap->GetResource(), mFrameResource.mBlurBufferH->GetResource());
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

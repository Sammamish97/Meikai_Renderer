#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "Camera.h"
#include "GeometryPass.h"
#include "LightingPass.h"
#include "SsaoPass.h"
#include "BlurPass.h"
#include "Texture.h"

#include <d3dcompiler.h>
#include <d3dx12.h>

#include "BlurPass.h"
#include "MathHelper.h"

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
	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	LoadContent();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();
	// Do the initial resize code.
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
	//BuildTextures();
	BuildModels();
	BuildFrameResource();
	CreateShader();
	G_Pass = std::make_unique<GeometryPass>(this, mCommandList.Get(), mShaders["GeomVS"], mShaders["GeomPS"], mClientWidth, mClientHeight);
	S_Pass = std::make_unique<SsaoPass>(this, mCommandList.Get(), mShaders["ScreenQuadVS"], mShaders["SsaoPS"], mClientWidth, mClientHeight);
	B_Pass = std::make_unique<BlurPass>(this, mCommandList.Get(), mShaders["HBlurCS"], mShaders["VBlurCS"], mClientWidth, mClientHeight);
	L_Pass = std::make_unique<LightingPass>(this, mCommandList.Get(), mShaders["ScreenQuadVS"], mShaders["LightingPS"], mClientWidth, mClientHeight);
	
	m_ContentLoaded = true; 
}

void Demo::BuildModels()
{
	mModels["Monkey"] = std::make_shared<Model>("../models/Monkey.obj", this, mCommandList);
	mModels["Quad"] = std::make_shared<Model>("../models/Quad.obj", this, mCommandList);
	mModels["Torus"] = std::make_shared<Model>("../models/Torus.obj", this, mCommandList);
	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", this, mCommandList);


	objects.push_back(std::make_unique<Object>(mModels["Plane"], XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(10.f, 10.f, 10.f)));
	objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(0.f, 0.f, 0.f)));

	//objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(-1.f, -1.f, 0.f)));
	//objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(1.f, -1.f, 0.f)));
}

void Demo::BuildTextures()
{
	test = std::make_unique<Texture>(this, mCommandList, "../textures/nice.jpg");
}

void Demo::BuildFrameResource()
{
	mFrameResource = std::make_unique<FrameResource>(mdxDevice.Get(), 1, 1);
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

	auto passCB = mFrameResource->mPassCB.get();
	passCB->CopyData(0, currentFrameCB);
}

void Demo::UpdateLightCB(const GameTimer& gt)
{
	LightCB lightData;

	lightData.directLight.Direction = XMFLOAT3(-0.5f, 0.5f, 0.5f);
	lightData.directLight.Color = XMFLOAT3(0.1f, 0.1f, 0.1f);

	lightData.pointLight[0].Position = XMFLOAT3(5, 0, 0);
	lightData.pointLight[0].Color = XMFLOAT3(0.5, 0, 0);

	lightData.pointLight[1].Position = XMFLOAT3(0, 5, 0);
	lightData.pointLight[1].Color = XMFLOAT3(0.1, 0.1, 0.1);

	lightData.pointLight[2].Position = XMFLOAT3(0, 0, 5);
	lightData.pointLight[2].Color = XMFLOAT3(0, 0, 0.5);

	auto LightCB = L_Pass->mLightCB.get();
	LightCB->CopyData(0, lightData);
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

	DrawGeometry(gt);
	DrawSsao(gt);
	BlurSsao(gt);
	DrawLighting(gt);

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

void Demo::DrawGeometry(const GameTimer& gt)
{
	auto positionMap = G_Pass->GetPositionMap();
	auto positionMapRtv = G_Pass->GetPosRtv();

	auto normalMap = G_Pass->GetNormalMap();
	auto normalMapRtv = G_Pass->GetNormalRtv();

	auto albedoMap = G_Pass->GetAlbedoMap();
	auto albedoMapRtv = G_Pass->GetAlbedoRtv();

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(positionMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(albedoMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float colorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float positionAndNormalClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCommandList->ClearRenderTargetView(positionMapRtv, positionAndNormalClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(normalMapRtv, positionAndNormalClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(albedoMapRtv, colorClearValue, 0, nullptr);

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvArray = { positionMapRtv, normalMapRtv, albedoMapRtv };

	mCommandList->ClearDepthStencilView(G_Pass->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//Set Pipeline & Root signature
	mCommandList->SetPipelineState(G_Pass->mPso.Get());
	mCommandList->SetGraphicsRootSignature(G_Pass->mRootSig.Get());

	//Update Pass CB
	UINT passCBByteSize = DxUtil::CalcConstantBufferByteSize(sizeof(PassCB));
	auto passCB = mFrameResource->mPassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(rtvArray.size(), rtvArray.data(),
		true, &G_Pass->DepthStencilView());

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : objects)
	{
		object->Draw(mCommandList);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(positionMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(albedoMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Demo::DrawSsao(const GameTimer& gt)
{
	auto SsaoMapResource = S_Pass->GetSsaoMap();
	auto SsaoRtv = S_Pass->GetSsaoRtv();

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(SsaoMapResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear ambient map
	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	mCommandList->ClearRenderTargetView(SsaoRtv, clearValue, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &SsaoRtv, true, nullptr);

	//Set Pipeline & Root signature
	mCommandList->SetPipelineState(S_Pass->mPso.Get());
	mCommandList->SetGraphicsRootSignature(S_Pass->mRootSig.Get());

	mCommandList->SetDescriptorHeaps(1, G_Pass->GetSrvHeap().GetAddressOf());

	//Access geometry pass's pos/normal/albedo map.
	mCommandList->SetGraphicsRootDescriptorTable(0, G_Pass->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	//Update Pass CB
	auto passCB = mFrameResource->mPassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	// Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->IASetVertexBuffers(0, 0, nullptr);
	mCommandList->IASetIndexBuffer(nullptr);
	mCommandList->DrawInstanced(3, 1, 0, 0);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(SsaoMapResource.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Demo::BlurSsao(const GameTimer& gt)
{
	auto hResource = B_Pass->GetHorizontalMap();
	auto vResource = B_Pass->GetVerticalMap();

	auto inputResource = S_Pass->GetSsaoMap();

	auto hGpuSrv = B_Pass->mBlurHGpuSrv;
	auto hGpuUav = B_Pass->mBlurHGpuUav;

	auto vGpuSrv = B_Pass->mBlurVGpuSrv;
	auto vGpuUav = B_Pass->mBlurVGpuUav;

	mCommandList->SetComputeRootSignature(B_Pass->mRootSig.Get());

	auto weights = S_Pass->CalcGaussWeights(2.5f);
	int blurRadius = (int)weights.size() / 2;

	mCommandList->SetComputeRoot32BitConstants(3, 1, &blurRadius, 0);
	mCommandList->SetComputeRoot32BitConstants(3, (UINT)weights.size(), weights.data(), 1);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hResource.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	// Copy the input (back-buffer in this example) to horizontal blur map.
	mCommandList->CopyResource(hResource.Get(), inputResource.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vResource.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	for (int i = 0; i < blurCount; ++i)
	{
		//
		// Horizontal Blur pass.
		//

		mCommandList->SetPipelineState(B_Pass->mhPso.Get());

		mCommandList->SetDescriptorHeaps(1, G_Pass->GetSrvHeap().GetAddressOf());

		mCommandList->SetComputeRootDescriptorTable(0, G_Pass->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

		mCommandList->SetDescriptorHeaps(1, B_Pass->GetSrvUavHeap().GetAddressOf());

		mCommandList->SetComputeRootDescriptorTable(1, hGpuSrv);
		mCommandList->SetComputeRootDescriptorTable(2, vGpuUav);

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		UINT numGroupsX = (UINT)ceilf(mClientWidth / 256.0f);
		mCommandList->Dispatch(numGroupsX, mClientHeight, 1);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hResource.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vResource.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		//
		// Vertical Blur pass.
		//

		mCommandList->SetPipelineState(B_Pass->mvPso.Get());

		mCommandList->SetDescriptorHeaps(1, G_Pass->GetSrvHeap().GetAddressOf());

		mCommandList->SetComputeRootDescriptorTable(0, G_Pass->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

		mCommandList->SetDescriptorHeaps(1, B_Pass->GetSrvUavHeap().GetAddressOf());

		mCommandList->SetComputeRootDescriptorTable(1, vGpuSrv);
		mCommandList->SetComputeRootDescriptorTable(2, hGpuUav);

		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		UINT numGroupsY = (UINT)ceilf(mClientHeight / 256.0f);
		mCommandList->Dispatch(mClientWidth, numGroupsY, 1);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hResource.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vResource.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputResource.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));

	// Copy the input (back-buffer in this example) to horizontal blur map.
	mCommandList->CopyResource(inputResource.Get(), hResource.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hResource.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Demo::DrawLighting(const GameTimer& gt)
{
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float positionAndcolorClearValue[] = { 1.f, 0.f, 1.f, 0.f };
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), positionAndcolorClearValue, 0, nullptr);

	//Set Pipeline & Root signature
	mCommandList->SetPipelineState(L_Pass->mPso.Get());
	mCommandList->SetGraphicsRootSignature(L_Pass->mRootSig.Get());

	mCommandList->SetDescriptorHeaps(1, G_Pass->GetSrvHeap().GetAddressOf());

	//Test descriptor heap accessing
	mCommandList->SetGraphicsRootDescriptorTable(0, G_Pass->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	D3D12_GPU_VIRTUAL_ADDRESS lightCBAddress = L_Pass->mLightCB->Resource()->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(1, lightCBAddress);

	mCommandList->SetDescriptorHeaps(1, S_Pass->GetSrvHeap().GetAddressOf());
	mCommandList->SetGraphicsRootDescriptorTable(2, S_Pass->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

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

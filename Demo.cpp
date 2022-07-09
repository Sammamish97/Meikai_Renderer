#include "Demo.h"
#include "DXUtil.h"
#include "Model.h"
#include "Object.h"
#include "Camera.h"
#include "GeometryPass.h"

#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <d3dx12.h>

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

	G_Pass = std::make_unique<GeometryPass>(mdxDevice.Get(), mCommandList.Get(), mClientWidth, mClientHeight);
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

D3D12_CPU_DESCRIPTOR_HANDLE Demo::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}


void Demo::OnResize()
{
	DXApp::OnResize();
	mDepthStencilBuffer.Reset();
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  s
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = mDepthStencilFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(mdxDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	mdxDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	ThrowIfFailed(mCommandList->Close())
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
}

void Demo::LoadContent()
{
	float aspectRatio = mClientWidth / static_cast<float>(mClientHeight);
	mCamera = std::make_unique<Camera>(aspectRatio);
	BuildModels();
	BuildFrameResource();
	CreateShader();
	CreateDsvDescriptorHeap();
	CreateGeometryRTV();
	BuildGeometryRootSignature();
	BuildGeometryPSO();

	m_ContentLoaded = true; 
}

void Demo::BuildGeometryRootSignature()
{
	//Geom use only 2 root parameter. One for model matrix 32-bit constant buffer, other for PassCB root descriptor.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(mdxDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	// Use for vertex shader only.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(1);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(mdxDevice->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(G_Pass->mRootSig.GetAddressOf())))
}

void Demo::BuildGeometryPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC geometryPSODesc;
	ZeroMemory(&geometryPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	geometryPSODesc.pRootSignature = G_Pass->mRootSig.Get();
	geometryPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["GeomVS"]->GetBufferPointer()),
		mShaders["GeomVS"]->GetBufferSize()
	};
	geometryPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["GeomPS"]->GetBufferPointer()),
		mShaders["GeomPS"]->GetBufferSize()
	};
	geometryPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	geometryPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	geometryPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	geometryPSODesc.SampleMask = UINT_MAX;
	geometryPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	geometryPSODesc.NumRenderTargets = 3;
	geometryPSODesc.RTVFormats[0] = G_Pass->PositionAndNormalMapFormat;
	geometryPSODesc.RTVFormats[1] = G_Pass->PositionAndNormalMapFormat;
	geometryPSODesc.RTVFormats[2] = G_Pass->AlbedoMapFormat;
	geometryPSODesc.DSVFormat = mDepthStencilFormat;
	geometryPSODesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	geometryPSODesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	geometryPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

	
	ThrowIfFailed(mdxDevice->CreateGraphicsPipelineState(&geometryPSODesc, IID_PPV_ARGS(G_Pass->mPso.GetAddressOf())))
}


void Demo::BuildModels()
{
	mModels["Monkey"] = std::make_shared<Model>("models/Monkey.obj", this, mCommandList);

	objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(0.f, 1.f, 0.f)));
	objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(-1.f, -1.f, 0.f)));
	objects.push_back(std::make_unique<Object>(mModels["Monkey"], XMFLOAT3(1.f, -1.f, 0.f)));
}

void Demo::BuildFrameResource()
{
	mFrameResource = std::make_unique<FrameResource>(mdxDevice.Get(), 1, 1);
}

void Demo::CreateGeometryRTV()
{
	// These three maps for geometry's position, normal and albedo.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdxDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mGeometryRtvHeap.GetAddressOf())));

	G_Pass->BuildDescriptors(GetGeometryRtvCpuHandle(0), mRtvDescriptorSize);
}

void Demo::CreateDsvDescriptorHeap()
{
	//Create RTV descriptor heap for depth stencil image
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;//TODO: For shadow map, Num descriptors need to be increase.
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(mdxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())))
}

void Demo::CreateShader()
{
	mShaders["standardVS"] = DxUtil::CompileShader(L"shaders/VertexShader.hlsl", nullptr, "main", "vs_5_1");
	mShaders["opaquePS"] = DxUtil::CompileShader(L"shaders/PixelShader.hlsl", nullptr, "main", "ps_5_1");
	mShaders["GeomVS"] = DxUtil::CompileShader(L"shaders/GeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["GeomPS"] = DxUtil::CompileShader(L"shaders/GeometryPass.hlsl", nullptr, "PS", "ps_5_1");
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Demo::GetGeometryRtvCpuHandle(int index)
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mGeometryRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
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

void Demo::Update(const GameTimer& gt)
{
	mCamera->Update(gt);
	UpdatePassCB(gt);
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
	float normalClearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	float positionAndcolorClearValue[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mCommandList->ClearRenderTargetView(positionMapRtv, positionAndcolorClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(normalMapRtv, normalClearValue, 0, nullptr);
	mCommandList->ClearRenderTargetView(albedoMapRtv, positionAndcolorClearValue, 0, nullptr);

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvArray = { positionMapRtv, normalMapRtv, albedoMapRtv };

	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

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
	mCommandList->OMSetRenderTargets(rtvArray.size(), rtvArray.data(), true, &DepthStencilView());

	//TODO: Draw이전 Pass별로 필요한 data(Matrix, Light, 기타등등 넘겨줘야함)

	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (const auto& object : objects)
	{
		object->Draw(mCommandList, XMLoadFloat4x4(&mCamera->GetViewMat()), XMLoadFloat4x4(&mCamera->GetProjMat()));
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(positionMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(albedoMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
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

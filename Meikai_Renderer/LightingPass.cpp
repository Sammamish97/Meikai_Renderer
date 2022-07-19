#include "LightingPass.h"
#include "DXApp.h"
#include "DXUtil.h"

LightingPass::LightingPass(DXApp* mApp, ComPtr<ID3D12GraphicsCommandList> cmdList,
	ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader, UINT width, UINT height)
	:mdxApp(mApp), mRenderTargetWidth(width), mRenderTargetHeight(height), mVertShader(vertShader), mPixelShader(pixelShader)
{
	BuildResource();
	BuildCbvheap();
	BuildCbvDesc();
	BuildRootSignature();
	BuildPSO();
	OnResize(width, height);
}

void LightingPass::OnResize(UINT newWidth, UINT newHeight)
{
    if (mRenderTargetWidth != newWidth || mRenderTargetHeight != newHeight)
    {
        mRenderTargetWidth = newWidth;
        mRenderTargetHeight = newHeight;

        // We render to ambient map at half the resolution.
        mViewport.TopLeftX = 0.0f;
        mViewport.TopLeftY = 0.0f;
        mViewport.Width = mRenderTargetWidth / 2.0f;
        mViewport.Height = mRenderTargetHeight / 2.0f;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;

        mScissorRect = { 0, 0, (int)mRenderTargetWidth / 2, (int)mRenderTargetHeight / 2 };
    }
}

void LightingPass::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lightingPSODesc;
	ZeroMemory(&lightingPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	lightingPSODesc.pRootSignature = mRootSig.Get();
	lightingPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
		mVertShader->GetBufferSize()
	};
	lightingPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
		mPixelShader->GetBufferSize()
	};
	lightingPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lightingPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	lightingPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lightingPSODesc.SampleMask = UINT_MAX;
	lightingPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lightingPSODesc.NumRenderTargets = 1;
	lightingPSODesc.RTVFormats[0] = mdxApp->GetBackBufferFormat();
	lightingPSODesc.SampleDesc.Count = mdxApp->Get4xMsaaState() ? 4 : 1;
	lightingPSODesc.SampleDesc.Quality = mdxApp->Get4xMsaaState() ? (mdxApp->Get4xMsaaQuality() - 1) : 0;

	lightingPSODesc.InputLayout = { nullptr, 0 };

	ThrowIfFailed(mdxApp->GetDevice()->CreateGraphicsPipelineState(&lightingPSODesc, IID_PPV_ARGS(mPso.GetAddressOf())))
}

void LightingPass::BuildResource()
{
	mLightCB = std::make_unique<UploadBuffer<LightCB>>(mdxApp->GetDevice().Get(), 1, true);
}

void LightingPass::BuildRootSignature()
{
	//Light pass use 3 textures for root value. But include depth, light pass use 4 textures.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(mdxApp->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
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

	CD3DX12_DESCRIPTOR_RANGE texTable0;//Table for position, normal, albedo texture of Geometry pass.
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstantBufferView(2);

	auto staticSamplers = mdxApp->GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(rootParameters), rootParameters,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(mdxApp->GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void LightingPass::BuildCbvheap()
{
	//One descriptor in the heap.
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(
		&cbvHeapDesc, IID_PPV_ARGS(mCbvHeap.GetAddressOf())))
}

void LightingPass::BuildCbvDesc()
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mLightCB->Resource()->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = mLightCB->GetSize();
	mdxApp->GetDevice()->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}
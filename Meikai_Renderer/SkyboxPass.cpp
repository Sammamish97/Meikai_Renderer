#include "SkyboxPass.h"
#include "DXApp.h"
#include "DXUtil.h"
#include "Texture.h"


#include <DDSTextureLoader.h>

SkyboxPass::SkyboxPass(DXApp* mApp,
	ComPtr<ID3D12GraphicsCommandList> cmdList,
	ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader,
	UINT width, UINT height)
	:mdxApp(mApp), mRenderTargetWidth(width), mRenderTargetHeight(height), mVertShader(vertShader), mPixelShader(pixelShader)
{
	BuildResource(cmdList);
	BuildSrvHeap();
	BuildSrvDesc();
	BuildRootSignature();
	BuildPSO();
	OnResize(width, height);
}

void SkyboxPass::OnResize(UINT newWidth, UINT newHeight)
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

void SkyboxPass::BuildResource(ComPtr<ID3D12GraphicsCommandList> cmdList)
{
	auto texMap = std::make_unique<Texture>();
	texMap->path = L"../textures/grassCube.dds";

	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mdxApp->GetDevice().Get(),
		cmdList.Get(), texMap->path.c_str(),
		texMap->resource, texMap->staging));

	mCubeTexture = std::move(texMap);
}

void SkyboxPass::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC SkyboxPSODesc;
	ZeroMemory(&SkyboxPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	SkyboxPSODesc.pRootSignature = mRootSig.Get();
	SkyboxPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
		mVertShader->GetBufferSize()
	};
	SkyboxPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
		mPixelShader->GetBufferSize()
	};
	SkyboxPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	SkyboxPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	SkyboxPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	SkyboxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	SkyboxPSODesc.SampleMask = UINT_MAX;
	SkyboxPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	SkyboxPSODesc.NumRenderTargets = 1;
	SkyboxPSODesc.RTVFormats[0] = mdxApp->GetBackBufferFormat();
	SkyboxPSODesc.DSVFormat = mDepthStencilDsvFormat;
	SkyboxPSODesc.SampleDesc.Count = mdxApp->Get4xMsaaState() ? 4 : 1;
	SkyboxPSODesc.SampleDesc.Quality = mdxApp->Get4xMsaaState() ? (mdxApp->Get4xMsaaQuality() - 1) : 0;

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	   {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	   {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	   {"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	   {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	   {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	SkyboxPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

	ThrowIfFailed(mdxApp->GetDevice()->CreateGraphicsPipelineState(&SkyboxPSODesc, IID_PPV_ARGS(mPso.GetAddressOf())))
}

void SkyboxPass::BuildRootSignature()
{
	//Skybox pass use 1 cubemap textures for root value. Also, need g-buffer's depth image for depth testing.
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

	CD3DX12_DESCRIPTOR_RANGE texTable0;//Table for cubemap
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].InitAsConstantBufferView(0);
	rootParameters[1].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);

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

void SkyboxPass::BuildSrvHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mCubeSrvHeap)));
}

void SkyboxPass::BuildSrvDesc()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCubeSrvHeap->GetCPUDescriptorHandleForHeapStart());

	auto skyboxResource = mCubeTexture->resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyboxResource->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyboxResource->GetDesc().Format;
	mdxApp->GetDevice()->CreateShaderResourceView(skyboxResource.Get(), &srvDesc, hDescriptor);
}

ComPtr<ID3D12DescriptorHeap> SkyboxPass::GetSrvHeap()
{
	return mCubeSrvHeap;
}
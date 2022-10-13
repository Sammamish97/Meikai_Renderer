#include "SsaoPass.h"
#include "DXUtil.h"
#include "DXApp.h"
#include "DescriptorHeap.h"
#include <DirectXMath.h>

#include "BufferFormat.h"

SsaoPass::SsaoPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:IPass(appPtr, vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
    mSsaoIndices.Pos = mApp->mDescIndex.mPositionDescSrvIdx;
    mSsaoIndices.Normal = mApp->mDescIndex.mNormalDescSrvIdx;
    mSsaoIndices.Depth = mApp->mDescIndex.mDepthStencilSrvIdx;
}

void SsaoPass::InitRootSignature()
{
	//SSAO Pass use these values
		//0. Common CB
		//1. Depth buffer SRV
    auto device = mApp->GetDevice();
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    UINT descriptorNumber = mApp->GetDescriptorHeap(SRV_2D)->GetDescriptorNum();

    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, descriptorNumber, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[3];
    rootParameters[0].InitAsConstantBufferView(0);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsConstants(mSsaoIndices.TexNum + 1, 1);


    auto staticSamplers = mApp->GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(rootParameters), rootParameters,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void SsaoPass::InitPSO()
{
    auto device = mApp->GetDevice();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC SsaoPSO;
    ZeroMemory(&SsaoPSO, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    SsaoPSO.pRootSignature = mRootSig.Get();
    SsaoPSO.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    SsaoPSO.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    SsaoPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    SsaoPSO.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    SsaoPSO.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    SsaoPSO.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    SsaoPSO.DepthStencilState.DepthEnable = false;
    SsaoPSO.DepthStencilState.StencilEnable = false;
    SsaoPSO.SampleMask = UINT_MAX;
    SsaoPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    SsaoPSO.NumRenderTargets = 0;
    SsaoPSO.SampleDesc.Count = mApp->Get4xMsaaState() ? 4 : 1;
    SsaoPSO.SampleDesc.Quality = mApp->Get4xMsaaState() ? (mApp->Get4xMsaaQuality() - 1) : 0;
    SsaoPSO.NumRenderTargets = 1;
    SsaoPSO.RTVFormats[0] = BackBufferFormat;

   
    SsaoPSO.InputLayout = { nullptr, 0 };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&SsaoPSO, IID_PPV_ARGS(mPSO.GetAddressOf())))
}

#include "LightingPass.h"
#include "DXUtil.h"
#include "DXApp.h"
#include "DescriptorHeap.h"
#include "BufferFormat.h"

#include <DirectXMath.h>


LightingPass::LightingPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader, UINT iblDiffuseIdx, UINT iblSpecularIdx)
	:IPass(appPtr, vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
    InitDescIndices();
    mLightDescIndices.IBL_DIFFUSE = iblDiffuseIdx;
    mLightDescIndices.IBL_SPECULAR = iblSpecularIdx;
}

void LightingPass::InitDescIndices()
{
    mLightDescIndices.Pos = mApp->mDescIndex.mPositionDescSrvIdx;
    mLightDescIndices.Normal = mApp->mDescIndex.mNormalDescSrvIdx;
    mLightDescIndices.Albedo = mApp->mDescIndex.mAlbedoDescSrvIdx;
    mLightDescIndices.Roughness = mApp->mDescIndex.mRoughnessDescSrvIdx;
    mLightDescIndices.Metalic = mApp->mDescIndex.mMetalicDescSrvIdx;
    mLightDescIndices.SSAO = mApp->mDescIndex.mSsaoDescSrvIdx;
    mLightDescIndices.ShadowDepth = mApp->mDescIndex.mShadowDepthSrvIdx;
}

void LightingPass::InitRootSignature()
{
	//Light Pass use these root parameters
    auto device = mApp->GetDevice();
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    UINT descriptorNumber = mApp->GetDescriptorHeap(SRV_2D)->GetDescriptorNum();

    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, descriptorNumber, 0, 0);
    
    CD3DX12_ROOT_PARAMETER rootParameters[6];
	rootParameters[0].InitAsConstantBufferView(0);
    rootParameters[1].InitAsConstantBufferView(1);
    rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsConstants(mLightDescIndices.TexNum + 1, 2);
    rootParameters[4].InitAsConstantBufferView(3);
    rootParameters[5].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 4, 0);


    auto staticSamplers = mApp->GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(rootParameters), rootParameters,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void LightingPass::InitPSO()
{
	//Light Pass use these render target view.
		//BackBuffer: 0
    auto device = mApp->GetDevice();

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
    lightingPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    lightingPSODesc.DepthStencilState.DepthEnable = false;
    lightingPSODesc.DepthStencilState.StencilEnable = false;
    lightingPSODesc.SampleMask = UINT_MAX;
    lightingPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    lightingPSODesc.NumRenderTargets = 1;
    lightingPSODesc.RTVFormats[0] = BackBufferFormat;
    lightingPSODesc.SampleDesc.Count = mApp->Get4xMsaaState() ? 4 : 1;
    lightingPSODesc.SampleDesc.Quality = mApp->Get4xMsaaState() ? (mApp->Get4xMsaaQuality() - 1) : 0;

    lightingPSODesc.InputLayout = { nullptr, 0 };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&lightingPSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}


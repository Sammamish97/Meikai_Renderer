#include "EquiRectToCubemapPass.h"

#include <DirectXMath.h>

#include "DXApp.h"
#include "DXUtil.h"

EquiRectToCubemapPass::EquiRectToCubemapPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader)
	:IPass(appPtr, computeShader)
{
	InitRootSignature();
	InitPSO();
}

void EquiRectToCubemapPass::InitRootSignature()
{
    //EquiRectToCubemap compute shader need these
    //Cubemap face's size for 32bit - CB : b0
    //Equirectanglular HDR 2DTexture for SRV : t0
    //Cubemap texture for UAV: t1
    //Sampler for load Equirectangular texture: s0
    
    auto device = mApp->GetDevice();
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    UINT descriptorNumber = mApp->GetCBVSRVUAVDescriptorNum();//Pos + Normal + Albedo + Roughness + Metalic + SSAO + Depth + test

    //CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    //srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0);//TODO: need fix!

    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, descriptorNumber, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);//TODO: need fix!

    CD3DX12_ROOT_PARAMETER rootParameters[3];
    rootParameters[0].InitAsConstants(1, 0);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);

    auto staticSamplers = mApp->GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription(_countof(rootParameters), rootParameters,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void EquiRectToCubemapPass::InitPSO()
{
    auto device = mApp->GetDevice();
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePSODesc = {};
    computePSODesc.CS =
    {
    reinterpret_cast<BYTE*>(mComputeShader->GetBufferPointer()),
    mComputeShader->GetBufferSize()
    };
    computePSODesc.pRootSignature = mRootSig.Get();
    ThrowIfFailed(device->CreateComputePipelineState(&computePSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}

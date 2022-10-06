#include "EquiRectToCubemapPass.h"

#include <DirectXMath.h>

#include "DescriptorHeap.h"
#include "DXApp.h"
#include "DXUtil.h"

EquiRectToCubemapPass::EquiRectToCubemapPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader, UINT hdrSrvIdx, UINT SkyboxCubemapUrvIdx, UINT CubemapUrvIdx)
	:IPass(appPtr, computeShader)
{
	InitRootSignature();
	InitPSO();
    mEquiRectDescIndices.HDR_SRV_2D = hdrSrvIdx;
    mEquiRectDescIndices.Cubemap_UAV_Skybox = SkyboxCubemapUrvIdx;
    mEquiRectDescIndices.Cubemap_UAV_HDR = CubemapUrvIdx;
}

void EquiRectToCubemapPass::InitRootSignature()
{
    //EquiRectToCubemap compute shader need these
    //Cubemap face's size for 32bit - CB : b0
    //Equirectanglular HDR 2DTexture for SRV : t0
    //Sky Cubemap Texture & Cubemap texture for UAV: t1
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

    UINT srvDescriptorNumber = mApp->GetDescriptorHeap(SRV_2D)->GetMaxDescriptors();
    UINT uavDescriptorNumber = mApp->GetDescriptorHeap(UAV_2D_ARRAY)->GetMaxDescriptors();

    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvDescriptorNumber, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavDescriptorNumber, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[4];
    rootParameters[0].InitAsConstants(1, 0);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsConstants(mEquiRectDescIndices.TexNum + 1, 1);

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

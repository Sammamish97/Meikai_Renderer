#include "SkyboxPass.h"
#include "DefaultPass.h"
#include "DXUtil.h"
#include "DXApp.h"
#include "BufferFormat.h"
#include "DescriptorHeap.h"
#include <DirectXMath.h>

SkyboxPass::SkyboxPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader, UINT skyboxSrvIdx)
	:IPass(appPtr, vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
    mSkyboxDescIndices.Skybox = skyboxSrvIdx;
}

void SkyboxPass::InitRootSignature()
{
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
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    UINT descriptorNumber = mApp->GetDescriptorHeap(SRV_CUBE)->GetDescriptorNum();

    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, descriptorNumber, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].InitAsConstantBufferView(0);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsConstants(mSkyboxDescIndices.TexNum + 1, 1);

    auto staticSamplers = mApp->GetStaticSamplers();

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_0(_countof(rootParameters), rootParameters, (UINT)staticSamplers.size(), staticSamplers.data(), rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void SkyboxPass::InitPSO()
{
    auto device = mApp->GetDevice();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyboxPSODesc;
    ZeroMemory(&skyboxPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    skyboxPSODesc.pRootSignature = mRootSig.Get();
    skyboxPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    skyboxPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    skyboxPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    skyboxPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    skyboxPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    skyboxPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    skyboxPSODesc.SampleMask = UINT_MAX;
    skyboxPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    skyboxPSODesc.NumRenderTargets = 1;
    skyboxPSODesc.RTVFormats[0] = BackBufferFormat;
    skyboxPSODesc.DSVFormat = DepthStencilDSVFormat;
    skyboxPSODesc.SampleDesc.Count = mApp->Get4xMsaaState() ? 4 : 1;
    skyboxPSODesc.SampleDesc.Quality = mApp->Get4xMsaaState() ? (mApp->Get4xMsaaQuality() - 1) : 0;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    skyboxPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&skyboxPSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}

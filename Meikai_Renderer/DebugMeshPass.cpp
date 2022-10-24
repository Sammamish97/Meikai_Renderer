#include "DebugMeshPass.h"
#include "DXUtil.h"
#include "DXApp.h"
#include "BufferFormat.h"
#include <DirectXMath.h>

DebugMeshPass::DebugMeshPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:IPass(appPtr, vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
}

void DebugMeshPass::InitRootSignature()
{
	//Joint debug do not need any special input.
	//Need only matrices.
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

    //Default pass use these values for shader
        //0. PassCB for other matrix.
        //1. World matrix for each object.

    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1);


    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_0(_countof(rootParameters), rootParameters,
        0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void DebugMeshPass::InitPSO()
{
	//Joint debug pass draw only point
    auto device = mApp->GetDevice();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC jointDebugPSODesc;
    ZeroMemory(&jointDebugPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    jointDebugPSODesc.pRootSignature = mRootSig.Get();
    jointDebugPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    jointDebugPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    jointDebugPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    jointDebugPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    jointDebugPSODesc.SampleMask = UINT_MAX;
    jointDebugPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    jointDebugPSODesc.NumRenderTargets = 1;
    jointDebugPSODesc.RTVFormats[0] = BackBufferFormat;
    jointDebugPSODesc.SampleDesc.Count = mApp->Get4xMsaaState() ? 4 : 1;
    jointDebugPSODesc.SampleDesc.Quality = mApp->Get4xMsaaState() ? (mApp->Get4xMsaaQuality() - 1) : 0;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    jointDebugPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&jointDebugPSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}

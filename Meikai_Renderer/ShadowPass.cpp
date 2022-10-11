#include "ShadowPass.h"

#include <DirectXMath.h>

#include "DXUtil.h"
#include "DXApp.h"
#include "BufferFormat.h"

ShadowPass::ShadowPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:IPass(appPtr, vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
}

void ShadowPass::InitRootSignature()
{
		//Shadow pass use these uniform values
		//0. model's world position
		//1. light MVP matrix
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

    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void ShadowPass::InitPSO()
{
	//Shadow pass use these render targets
		//0. Depth buffer
    auto device = mApp->GetDevice();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPSODesc;
    ZeroMemory(&shadowPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    shadowPSODesc.pRootSignature = mRootSig.Get();
    shadowPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    shadowPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    shadowPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    shadowPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    shadowPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    shadowPSODesc.SampleMask = UINT_MAX;
    shadowPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    shadowPSODesc.NumRenderTargets = 0;
    shadowPSODesc.DSVFormat = DepthStencilDSVFormat;
    shadowPSODesc.SampleDesc.Count = mApp->Get4xMsaaState() ? 4 : 1;
    shadowPSODesc.SampleDesc.Quality = mApp->Get4xMsaaState() ? (mApp->Get4xMsaaQuality() - 1) : 0;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    shadowPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&shadowPSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}

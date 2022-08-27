#include "LightingPass.h"
#include "DXUtil.h"
#include "DXApp.h"
#include <DirectXMath.h>

#include "BufferFormat.h"

LightingPass::LightingPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:IPass(appPtr, vertShader, pixelShader)
{
	InitRootSignature();
	InitPSO();
}

void LightingPass::InitRootSignature()
{
	//Light Pass use these root parameters
		//PassCB: 0
		//Geometry textures: 1 ~ unbound
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
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    //Use descriptor table with size 1000 for use bindless descriptor.
    UINT descriptorNumber = 1000;
    CD3DX12_DESCRIPTOR_RANGE1 unboundedRange = {};
    unboundedRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, descriptorNumber, 0, 0);
    
    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstantBufferView(0);
    rootParameters[1].InitAsDescriptorTable(descriptorNumber, &unboundedRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
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
    lightingPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    lightingPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    lightingPSODesc.SampleMask = UINT_MAX;
    lightingPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    lightingPSODesc.NumRenderTargets = 1;
    lightingPSODesc.RTVFormats[0] = BackBufferFormat;
    lightingPSODesc.SampleDesc.Count = mApp->Get4xMsaaState() ? 4 : 1;
    lightingPSODesc.SampleDesc.Quality = mApp->Get4xMsaaState() ? (mApp->Get4xMsaaQuality() - 1) : 0;

    lightingPSODesc.InputLayout = { nullptr, 0 };

    ThrowIfFailed(device->CreateGraphicsPipelineState(&lightingPSODesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}
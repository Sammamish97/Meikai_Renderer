#include "BlurPassV.h"
#include "DescriptorHeap.h"
#include "DXApp.h"
#include "DXUtil.h"

BlurPassV::BlurPassV(DXApp* appPtr, ComPtr<ID3DBlob> computeShader)
	:IPass(appPtr, computeShader)
{
	InitRootSignature();
	InitPSO();
}

void BlurPassV::InitRootSignature()
{
    //Blur pass use these root values
        //0. 2DSRV table
        //1. 2DUAV table

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(mApp->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    UINT SrvDescriptorNumber = mApp->GetDescriptorHeap(SRV_2D)->GetDescriptorNum();
    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SrvDescriptorNumber, 0, 0);

    UINT UavDescriptorNumber = mApp->GetDescriptorHeap(UAV_2D)->GetDescriptorNum();
    CD3DX12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, UavDescriptorNumber, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[4];
    rootParameters[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsConstantBufferView(0);
    rootParameters[3].InitAsConstants(mBlurDescIndices.TexNum + 1, 1);

    auto staticSamplers = mApp->GetStaticSamplers();

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

    ThrowIfFailed(mApp->GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void BlurPassV::InitPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC blurPso = {};
    blurPso.pRootSignature = mRootSig.Get();
    blurPso.CS =
    {
        reinterpret_cast<BYTE*>(mComputeShader->GetBufferPointer()),
        mComputeShader->GetBufferSize()
    };
    blurPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(mApp->GetDevice()->CreateComputePipelineState(&blurPso, IID_PPV_ARGS(mPSO.GetAddressOf())))
}

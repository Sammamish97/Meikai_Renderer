#include "BlurPass.h"
#include "DXApp.h"
#include "DescriptorHeap.h"
#include "DXUtil.h"

BlurPass::BlurPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader, bool isHorizontal)
	:IPass(appPtr, computeShader)
{
	InitRootSignature();
	InitPSO();
    if(isHorizontal)
    {
        mBlurDescIndices.SRVTextureInput = mApp->mDescIndex.mBlurHSrvIdx;
        mBlurDescIndices.UAVTextureOutput = mApp->mDescIndex.mBlurVUavIdx;
    }
    else
    {
        mBlurDescIndices.SRVTextureInput = mApp->mDescIndex.mBlurVSrvIdx;
        mBlurDescIndices.UAVTextureOutput = mApp->mDescIndex.mBlurHUavIdx;
    }

    mBlurDescIndices.Normal = mApp->mDescIndex.mNormalDescSrvIdx;
    mBlurDescIndices.Depth = mApp->mDescIndex.mDepthStencilSrvIdx;
}

void BlurPass::InitRootSignature()
{
    auto device = mApp->GetDevice();
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    UINT srvDescriptorNumber = mApp->GetDescriptorHeap(SRV_2D)->GetMaxDescriptors();
    UINT uavDescriptorNumber = mApp->GetDescriptorHeap(UAV_2D)->GetMaxDescriptors();

    CD3DX12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvDescriptorNumber, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavDescriptorNumber, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[4];
    rootParameters[0].InitAsConstants(12, 0);
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsConstants(mBlurDescIndices.TexNum + 1, 1);

    auto staticSamplers = mApp->GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription(_countof(rootParameters), rootParameters,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void BlurPass::InitPSO()
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

std::vector<float> BlurPass::CalcGaussWeights(float sigma)
{
    float twoSigma2 = 2.0f * sigma * sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    int blurRadius = (int)ceil(2.0f * sigma);

    assert(blurRadius <= 5);

    std::vector<float> weights;
    weights.resize(2 * blurRadius + 1);

    float weightSum = 0.0f;

    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = (float)i;

        weights[i + blurRadius] = expf(-x * x / twoSigma2);

        weightSum += weights[i + blurRadius];
    }

    // Divide by the sum so all the weights add up to 1.0.
    for (int i = 0; i < weights.size(); ++i)
    {
        weights[i] /= weightSum;
    }

    return weights;
}

#include <DirectXMath.h>

#include "SsaoPass.h"
#include "DXUtil.h"
#include "DXApp.h"

SsaoPass::SsaoPass(DXApp* device, ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3DBlob> vertShader,
	ComPtr<ID3DBlob> pixelShader, UINT width, UINT height)
		:mdxApp(device), mRenderTargetWidth(width), mRenderTargetHeight(height), mVertShader(vertShader), mPixelShader(pixelShader)
{
	BuildMapResource();
    CreateRtvSrvDescHeap();
    BuildDescriptors();
	BuildRootSignature();
	BuildPSO();
	OnResize(width, height);
}

void SsaoPass::OnResize(UINT newWidth, UINT newHeight)
{
    if (mRenderTargetWidth != newWidth || mRenderTargetHeight != newHeight)
    {
        mRenderTargetWidth = newWidth;
        mRenderTargetHeight = newHeight;

        // We render to ambient map at half the resolution.
        mViewport.TopLeftX = 0.0f;
        mViewport.TopLeftY = 0.0f;
        mViewport.Width = mRenderTargetWidth / 2.0f;
        mViewport.Height = mRenderTargetHeight / 2.0f;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;

        mScissorRect = { 0, 0, (int)mRenderTargetWidth / 2, (int)mRenderTargetHeight / 2 };
    }
}

void SsaoPass::BuildMapResource()
{
    mSsaoMap.Reset();

    D3D12_RESOURCE_DESC texDesc = {};
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mRenderTargetWidth;
    texDesc.Height = mRenderTargetHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = SsaoMapFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float SsaoClearColor[] = { 0 };
    CD3DX12_CLEAR_VALUE ssaoOptClear(SsaoMapFormat, SsaoClearColor);
    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &ssaoOptClear,
        IID_PPV_ARGS(mSsaoMap.GetAddressOf())))
}

void SsaoPass::CreateRtvSrvDescHeap()
{
    //Ssao map contain only one texture which only have one color.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(
        &srvHeapDesc, IID_PPV_ARGS(mSrvHeap.GetAddressOf())))
}

void SsaoPass::BuildDescriptors()
{
    mhSsaoMapCpuSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
    mhSsaoMapGpuSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
    mhSsaoMapCpuRtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

    RebuildDescriptors();
}

void SsaoPass::RebuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc= {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = SsaoMapFormat;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    mdxApp->GetDevice()->CreateShaderResourceView(mSsaoMap.Get(), &srvDesc, mhSsaoMapCpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = SsaoMapFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    mdxApp->GetDevice()->CreateRenderTargetView(mSsaoMap.Get(), &rtvDesc, mhSsaoMapCpuRtv);
}

void SsaoPass::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPSODesc;
    ZeroMemory(&ssaoPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    ssaoPSODesc.pRootSignature = mRootSig.Get();
    ssaoPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    ssaoPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    ssaoPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    ssaoPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    ssaoPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    ssaoPSODesc.SampleMask = UINT_MAX;
    ssaoPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ssaoPSODesc.NumRenderTargets = 1;
    ssaoPSODesc.RTVFormats[0] = SsaoMapFormat;
    ssaoPSODesc.SampleDesc.Count = mdxApp->Get4xMsaaState() ? 4 : 1;
    ssaoPSODesc.SampleDesc.Quality = mdxApp->Get4xMsaaState() ? (mdxApp->Get4xMsaaQuality() - 1) : 0;

    ssaoPSODesc.InputLayout = { nullptr, 0 };

    ThrowIfFailed(mdxApp->GetDevice()->CreateGraphicsPipelineState(&ssaoPSODesc, IID_PPV_ARGS(mPso.GetAddressOf())))
}

void SsaoPass::BuildRootSignature()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(mdxApp->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    // Use for vertex shader only.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE texTable0;//Table for position, normal, albedo texture of Geometry pass.
    texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsConstantBufferView(0);

    auto staticSamplers = mdxApp->GetStaticSamplers();

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

    ThrowIfFailed(mdxApp->GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

ComPtr<ID3D12DescriptorHeap> SsaoPass::GetSrvHeap()
{
    return mSrvHeap;
}

ComPtr<ID3D12Resource> SsaoPass::GetSsaoMap()
{
    return mSsaoMap;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SsaoPass::GetSsaoRtv()
{
    return mhSsaoMapCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SsaoPass::GetSsaoSrv()
{
    return mhSsaoMapGpuSrv;
}

std::vector<float> SsaoPass::CalcGaussWeights(float sigma)
{
    float twoSigma2 = 2.0f * sigma * sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    int blurRadius = (int)ceil(2.0f * sigma);

    assert(blurRadius <= MaxBlurRadius);

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

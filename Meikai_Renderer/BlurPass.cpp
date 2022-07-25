#include "BlurPass.h"

#include "DXUtil.h"
#include "DXApp.h"

BlurPass::BlurPass(DXApp* device, ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3DBlob> hBlurShader,
	ComPtr<ID3DBlob> vBlurShader, UINT blurwWidth, UINT blurHeight)
    :mdxApp(device), mBlurWidth(blurwWidth), mBlurHeight(blurHeight),
		mhBlurShader(hBlurShader), mvBlurShader(vBlurShader)
{
    BuildBlurResource();
    CreateSrvUavDescriptorHeap();
    BuildDescriptors();
    BuildRootSignature();
    BuildBlurPSOs();
}

void BlurPass::BuildBlurResource()
{
	mBlurHorizon.Reset();
	mBlurVertical.Reset();

	D3D12_RESOURCE_DESC texDesc = {};
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mBlurWidth;
    texDesc.Height = mBlurHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = blurMapFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(mBlurHorizon.GetAddressOf())))

    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(mBlurVertical.GetAddressOf())))
}

void BlurPass::CreateSrvUavDescriptorHeap()
{
    //Blur pass use 4 descriptors of UAV/CBV/SRV
    //2 for horizontal's srv/uav
    //2 for vertical's srv/uav

    D3D12_DESCRIPTOR_HEAP_DESC SrvUavHeapDesc = {};
    SrvUavHeapDesc.NumDescriptors = 4;
    SrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    SrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(
        &SrvUavHeapDesc, IID_PPV_ARGS(mSrvUavHeap.GetAddressOf())))
}

void BlurPass::BuildDescriptors()
{
    auto cpuDescStart = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvUavHeap->GetCPUDescriptorHandleForHeapStart());
    auto gpuDescStart = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvUavHeap->GetGPUDescriptorHandleForHeapStart());
    auto SrvUavDescSize = mdxApp->GetCbvSrvUavDescSize();
    // Save references to the descriptors. 
    mBlurHCpuSrv = cpuDescStart;
    mBlurHCpuUav = cpuDescStart.Offset(1, SrvUavDescSize);
    mBlurVCpuSrv = cpuDescStart.Offset(1, SrvUavDescSize);
    mBlurVCpuUav = cpuDescStart.Offset(1, SrvUavDescSize);

    mBlurHGpuSrv = gpuDescStart;
    mBlurHGpuUav = gpuDescStart.Offset(1, SrvUavDescSize);
    mBlurVGpuSrv = gpuDescStart.Offset(1, SrvUavDescSize);
    mBlurVGpuUav = gpuDescStart.Offset(1, SrvUavDescSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = blurMapFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = blurMapFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    mdxApp->GetDevice()->CreateShaderResourceView(mBlurHorizon.Get(), &srvDesc, mBlurHCpuSrv);
    mdxApp->GetDevice()->CreateUnorderedAccessView(mBlurHorizon.Get(), nullptr, &uavDesc, mBlurHCpuUav);

    mdxApp->GetDevice()->CreateShaderResourceView(mBlurVertical.Get(), &srvDesc, mBlurVCpuSrv);
    mdxApp->GetDevice()->CreateUnorderedAccessView(mBlurVertical.Get(), nullptr, &uavDesc, mBlurVCpuUav);
}

void BlurPass::BuildRootSignature()
{
    //Blur compute shader use geometry pass's position, normal, albedo, depth buffer and input srv image.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(mdxApp->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE GBufferTable;//Table for position, normal, albedo and depth texture of Geometry pass.
    GBufferTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE InputSrvTable;//Table for input blur image
    InputSrvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);

    CD3DX12_DESCRIPTOR_RANGE OutputUAVTable;//Table for input blur image
    OutputUAVTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[4];
    rootParameters[0].InitAsDescriptorTable(1, &GBufferTable, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &InputSrvTable, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, &OutputUAVTable, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[3].InitAsConstants(12, 0);

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
    ThrowIfFailed(hr)

    ThrowIfFailed(mdxApp->GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void BlurPass::BuildBlurPSOs()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC blurPso = {};
    blurPso.pRootSignature = mRootSig.Get();
    blurPso.CS =
    {
        reinterpret_cast<BYTE*>(mhBlurShader->GetBufferPointer()),
        mhBlurShader->GetBufferSize()
    };
    blurPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(mdxApp->GetDevice()->CreateComputePipelineState(&blurPso, IID_PPV_ARGS(mhPso.GetAddressOf())))

	blurPso.CS =
    {
        reinterpret_cast<BYTE*>(mvBlurShader->GetBufferPointer()),
        mvBlurShader->GetBufferSize()
    };
    ThrowIfFailed(mdxApp->GetDevice()->CreateComputePipelineState(&blurPso, IID_PPV_ARGS(mvPso.GetAddressOf())))
}

ComPtr<ID3D12DescriptorHeap> BlurPass::GetSrvUavHeap()
{
    return mSrvUavHeap;
}

ComPtr<ID3D12Resource> BlurPass::GetHorizontalMap()
{
    return mBlurHorizon;
}

ComPtr<ID3D12Resource> BlurPass::GetVerticalMap()
{
    return mBlurVertical;
}

#include "BlurPass.h"

#include "DXUtil.h"
#include "DXApp.h"

BlurPass::BlurPass(DXApp* device,
	ComPtr<ID3D12GraphicsCommandList> cmdList,
	ComPtr<ID3DBlob> computeShader,
	UINT blurwWidth, UINT blurHeight)
	:mdxApp(device), mBlurWidth(blurwWidth), mBlurHeight(blurHeight), mComputeShader(computeShader)
{
	BuildBlurResource();
	CreateSrvUavDescriptorHeap();
	BuildDescriptors();
	BuildRootSignature();
	BuildComputePSO();
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
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float blurMapClearColor[] = { 0 };
    CD3DX12_CLEAR_VALUE blurOptClear(blurMapFormat, blurMapClearColor);
    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &blurOptClear,
        IID_PPV_ARGS(mBlurHorizon.GetAddressOf())))

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
    //TODO: start here
}

void BlurPass::BuildComputePSO()
{

}



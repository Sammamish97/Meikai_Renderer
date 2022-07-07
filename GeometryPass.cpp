#include "GeometryPass.h"
#include "DXUtil.h"

GeometryPass::GeometryPass(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList, UINT width,
                           UINT height)
		:mdxDevice(device)
{
	OnResize(width, height);
}

void GeometryPass::OnResize(UINT newWidth, UINT newHeight)
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

        BuildResources();
    }
}

void GeometryPass::SetPSOs()
{
}

void GeometryPass::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT cbvSrvUavDescriptorSize, UINT rtvDescriptorSize)
{
    mhPositionMapCpuSrv = hCpuSrv;
    mhNormalMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhAlbedoMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);;

    mhPositionMapGpuSrv = hGpuSrv;
    mhNormalMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhAlbedoMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);;

    mhPositionMapCpuRtv = hCpuRtv;
    mhNormalMapCpuRtv = hCpuRtv;
    mhAlbedoMapCpuRtv = hCpuRtv;

    RebuildDescriptors();
}

void GeometryPass::RebuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = PositionAndNormalMapFormat;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    mdxDevice->CreateShaderResourceView(mPositionMap.Get(), &srvDesc, mhPositionMapCpuSrv);
    mdxDevice->CreateShaderResourceView(mNormalMap.Get(), &srvDesc, mhNormalMapCpuSrv);

    srvDesc.Format = AlbedoMapFormat;
    mdxDevice->CreateShaderResourceView(mAlbedoMap.Get(), &srvDesc, mhAlbedoMapCpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = PositionAndNormalMapFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    mdxDevice->CreateRenderTargetView(mPositionMap.Get(), &rtvDesc, mhPositionMapCpuRtv);
    mdxDevice->CreateRenderTargetView(mNormalMap.Get(), &rtvDesc, mhNormalMapCpuRtv);


    rtvDesc.Format = AlbedoMapFormat;
    mdxDevice->CreateRenderTargetView(mAlbedoMap.Get(), &rtvDesc, mhAlbedoMapCpuRtv);
}

void GeometryPass::BuildResources()
{
    mPositionMap.Reset();
    mNormalMap.Reset();
    mAlbedoMap.Reset();

    D3D12_RESOURCE_DESC texDesc = {};
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mRenderTargetWidth;
    texDesc.Height = mRenderTargetHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = PositionAndNormalMapFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float positionClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    CD3DX12_CLEAR_VALUE posOptClear(PositionAndNormalMapFormat, positionClearColor);
    ThrowIfFailed(mdxDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &posOptClear,
        IID_PPV_ARGS(mPositionMap.GetAddressOf())))
    

    float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    CD3DX12_CLEAR_VALUE normalOptClear(PositionAndNormalMapFormat, normalClearColor);
    ThrowIfFailed(mdxDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &normalOptClear,
        IID_PPV_ARGS(mNormalMap.GetAddressOf())))

    texDesc.Format = AlbedoMapFormat;
    float alBedoClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    CD3DX12_CLEAR_VALUE albedoOptClear(AlbedoMapFormat, alBedoClearColor);
    ThrowIfFailed(mdxDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &albedoOptClear,
        IID_PPV_ARGS(mAlbedoMap.GetAddressOf())))
}

ComPtr<ID3D12Resource> GeometryPass::GetPositionMap()
{
    return mPositionMap;
}

ComPtr<ID3D12Resource> GeometryPass::GetNormalMap()
{
    return mNormalMap;
}

ComPtr<ID3D12Resource> GeometryPass::GetAlbedoMap()
{
    return mAlbedoMap;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GeometryPass::GetPosRtv()
{
    return mhPositionMapCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GeometryPass::GetPosSrv()
{
    return mhPositionMapGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GeometryPass::GetNormalRtv()
{
    return mhNormalMapCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GeometryPass::GetNormalSrv()
{
    return mhNormalMapGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GeometryPass::GetAlbedoRtv()
{
    return mhAlbedoMapCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GeometryPass::GetAlbedoSrv()
{
    return mhAlbedoMapGpuSrv;
}

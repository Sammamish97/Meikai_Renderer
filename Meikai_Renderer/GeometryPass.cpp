#include "GeometryPass.h"

#include <DirectXMath.h>

#include "DXUtil.h"
#include "DXApp.h"

GeometryPass::GeometryPass(DXApp* device, ComPtr<ID3D12GraphicsCommandList> cmdList,
	ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader, UINT width, UINT height)
    :mdxApp(device), mRenderTargetWidth(width), mRenderTargetHeight(height), mVertShader(vertShader), mPixelShader(pixelShader)
{
    BuildRTVResources();
    BuildDSVResource();
    CreateRtvDescHeap();
    CreateDsvDescHeap();
    BuildRootSignature();
    BuildPSO();
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
    }
}

void GeometryPass::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT cbvSrvUavDescriptorSize, UINT rtvDescriptorSize)
{
    mhPositionMapCpuSrv = hCpuSrv;
    mhNormalMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
	mhAlbedoMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhDepthCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    mhPositionMapGpuSrv = hGpuSrv;
    mhNormalMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhAlbedoMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhDepthGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    mhPositionMapCpuRtv = hCpuRtv;
    mhNormalMapCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);
    mhAlbedoMapCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);

    RebuildDescriptors();
}

void GeometryPass::BuildDsvDescriptor()
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilDsvFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    mdxApp->GetDevice()->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());
}

void GeometryPass::RebuildDescriptors()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = PositionAndNormalMapFormat;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    mdxApp->GetDevice()->CreateShaderResourceView(mPositionMap.Get(), &srvDesc, mhPositionMapCpuSrv);
    mdxApp->GetDevice()->CreateShaderResourceView(mNormalMap.Get(), &srvDesc, mhNormalMapCpuSrv);

    srvDesc.Format = AlbedoMapFormat;
    mdxApp->GetDevice()->CreateShaderResourceView(mAlbedoMap.Get(), &srvDesc, mhAlbedoMapCpuSrv);

    srvDesc.Format = mDepthStencilSrvFormat;
    mdxApp->GetDevice()->CreateShaderResourceView(mDepthStencilBuffer.Get(), &srvDesc, mhDepthCpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = PositionAndNormalMapFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    mdxApp->GetDevice()->CreateRenderTargetView(mPositionMap.Get(), &rtvDesc, mhPositionMapCpuRtv);
    mdxApp->GetDevice()->CreateRenderTargetView(mNormalMap.Get(), &rtvDesc, mhNormalMapCpuRtv);


    rtvDesc.Format = AlbedoMapFormat;
    mdxApp->GetDevice()->CreateRenderTargetView(mAlbedoMap.Get(), &rtvDesc, mhAlbedoMapCpuRtv);
}

void GeometryPass::BuildRTVResources()
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
    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &posOptClear,
        IID_PPV_ARGS(mPositionMap.GetAddressOf())))
    

    float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    CD3DX12_CLEAR_VALUE normalOptClear(PositionAndNormalMapFormat, normalClearColor);
    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &normalOptClear,
        IID_PPV_ARGS(mNormalMap.GetAddressOf())))

    texDesc.Format = AlbedoMapFormat;
    float alBedoClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    CD3DX12_CLEAR_VALUE albedoOptClear(AlbedoMapFormat, alBedoClearColor);
    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &albedoOptClear,
        IID_PPV_ARGS(mAlbedoMap.GetAddressOf())))

}

void GeometryPass::BuildDSVResource()
{
    mDepthStencilBuffer.Reset();
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mdxApp->GetWindowSize().first;
    depthStencilDesc.Height = mdxApp->GetWindowSize().second;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

    // Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
    // the depth buffer.  Therefore, because we need to create two views to the same resource:
    //   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    //   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
    // we need to create the depth buffer resource with a typeless format.  s
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = mdxApp->Get4xMsaaState() ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = mdxApp->Get4xMsaaState() ? (mdxApp->Get4xMsaaQuality() - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = mDepthStencilDsvFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(mdxApp->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &clearValue,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))
    //TODO: state를 D3D12_RESOURCE_STATE_COMMON에서 Depth write로 바꾸기.
}

void GeometryPass::CreateRtvDescHeap()
{
    // These three maps for geometry's position, normal and albedo.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = 3;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mGeometryRtvHeap.GetAddressOf())));

    //TODO: Detatch Rtv & Srv heap & Desc from this function for intuitivity. 
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 4;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(
        &srvHeapDesc, IID_PPV_ARGS(mSrvHeap.GetAddressOf())))

	BuildDescriptors(GetCpuSrv(0), GetGpuSrv(0), GetGeometryRtvCpuHandle(0),
        mdxApp->GetCbvSrvUavDescSize(), mdxApp->GetRtvDescSize());
}

void GeometryPass::CreateDsvDescHeap()
{
    //Create RTV descriptor heap for depth stencil image
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;//TODO: For shadow map, Num descriptors need to be increase.
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(mdxApp->GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())))

	BuildDsvDescriptor();
}

void GeometryPass::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC geometryPSODesc;
    ZeroMemory(&geometryPSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    geometryPSODesc.pRootSignature = mRootSig.Get();
    geometryPSODesc.VS =
    {
        reinterpret_cast<BYTE*>(mVertShader->GetBufferPointer()),
        mVertShader->GetBufferSize()
    };
    geometryPSODesc.PS =
    {
        reinterpret_cast<BYTE*>(mPixelShader->GetBufferPointer()),
        mPixelShader->GetBufferSize()
    };
    geometryPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    geometryPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    geometryPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    geometryPSODesc.SampleMask = UINT_MAX;
    geometryPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    geometryPSODesc.NumRenderTargets = 3;
    geometryPSODesc.RTVFormats[0] = PositionAndNormalMapFormat;
    geometryPSODesc.RTVFormats[1] = PositionAndNormalMapFormat;
    geometryPSODesc.RTVFormats[2] = AlbedoMapFormat;
    geometryPSODesc.DSVFormat = mDepthStencilDsvFormat;
    geometryPSODesc.SampleDesc.Count = mdxApp->Get4xMsaaState() ? 4 : 1;
    geometryPSODesc.SampleDesc.Quality = mdxApp->Get4xMsaaState() ? (mdxApp->Get4xMsaaQuality() - 1) : 0;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    geometryPSODesc.InputLayout = { inputLayout, _countof(inputLayout) };

    ThrowIfFailed(mdxApp->GetDevice()->CreateGraphicsPipelineState(&geometryPSODesc, IID_PPV_ARGS(mPso.GetAddressOf())))
}

void GeometryPass::BuildRootSignature()
{
    //Geom use only 2 root parameter. One for model matrix 32-bit constant buffer, other for PassCB root descriptor.
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

    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstantBufferView(1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
        ThrowIfFailed(mdxApp->GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GeometryPass::GetGeometryRtvCpuHandle(int index)
{
    auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mGeometryRtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtv.Offset(index, mdxApp->GetRtvDescSize());
    return rtv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GeometryPass::GetCpuSrv(int index)
{
    auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
    srv.Offset(index, mdxApp->GetCbvSrvUavDescSize());
    return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GeometryPass::GetGpuSrv(int index)
{
    auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
    srv.Offset(index, mdxApp->GetCbvSrvUavDescSize());
    return srv;
}

ComPtr<ID3D12DescriptorHeap> GeometryPass::GetSrvHeap()
{
    return mSrvHeap;
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

D3D12_CPU_DESCRIPTOR_HANDLE GeometryPass::DepthStencilView()
{
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

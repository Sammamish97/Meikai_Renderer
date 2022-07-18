#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;
class DXApp;
class GeometryPass
{
	DXApp* mdxApp = nullptr;
public:
	GeometryPass(DXApp* device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader,
		UINT width, UINT height);
	GeometryPass(const GeometryPass& rhs) = delete;
	GeometryPass& operator=(const GeometryPass& rhs) = delete;
	~GeometryPass() = default;

	static const DXGI_FORMAT PositionAndNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	static const DXGI_FORMAT AlbedoMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

private:
	void BuildRTVResources();
	void BuildDSVResource();

	void CreateRtvDescHeap();
	void CreateDsvDescHeap();

	void BuildPSO();
	void BuildRootSignature();

public:
	void OnResize(UINT newWidth, UINT newHeight);

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT cbvSrvUavDescriptorSize,
		UINT rtvDescriptorSize);

	void BuildDsvDescriptor();

	void RebuildDescriptors();

	ComPtr<ID3D12Resource> GetPositionMap();
	ComPtr<ID3D12Resource> GetNormalMap();
	ComPtr<ID3D12Resource> GetAlbedoMap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetPosRtv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetPosSrv();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetNormalRtv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetNormalSrv();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetAlbedoRtv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetAlbedoSrv();

	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetGeometryRtvCpuHandle(int index);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index);

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();

private:
	ComPtr<ID3D12DescriptorHeap> mGeometryRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	ComPtr<ID3D12DescriptorHeap> mSrvHeap;

	ComPtr<ID3D12Resource> mPositionMap;
	ComPtr<ID3D12Resource> mNormalMap;
	ComPtr<ID3D12Resource> mAlbedoMap;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhPositionMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionMapCpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAlbedoMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhAlbedoMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhAlbedoMapCpuRtv;

	UINT mRenderTargetWidth;
	UINT mRenderTargetHeight;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	ComPtr<ID3DBlob> mVertShader;
	ComPtr<ID3DBlob> mPixelShader;

public:
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPso;
};


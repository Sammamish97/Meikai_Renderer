#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;
class GeometryPass
{
public:
	GeometryPass(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		UINT width, UINT height);
	GeometryPass(const GeometryPass& rhs) = delete;
	GeometryPass& operator=(const GeometryPass& rhs) = delete;
	~GeometryPass() = default;

	static const DXGI_FORMAT PositionAndNormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	static const DXGI_FORMAT AlbedoMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void BuildResources();

public:
	void OnResize(UINT newWidth, UINT newHeight);

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT rtvDescriptorSize);

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

private:
	ComPtr<ID3D12Device> mdxDevice;

	ComPtr<ID3D12Resource> mPositionMap;
	ComPtr<ID3D12Resource> mNormalMap;
	ComPtr<ID3D12Resource> mAlbedoMap;

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

public:
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPso;
};


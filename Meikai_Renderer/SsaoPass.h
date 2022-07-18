#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;
class DXApp;
class SsaoPass
{
	DXApp* mdxApp = nullptr;
public:
	SsaoPass(DXApp* device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader,
		UINT width, UINT height);
	SsaoPass(const SsaoPass& rhs) = delete;
	SsaoPass& operator=(const SsaoPass& rhs) = delete;
	~SsaoPass() = default;

	static const DXGI_FORMAT SsaoMapFormat = DXGI_FORMAT_R16_UNORM;

private:
	void BuildMapResource();

	void CreateRtvSrvDescHeap();

	void BuildPSO();
	void BuildRootSignature();

public:
	void OnResize(UINT newWidth, UINT newHeight);

	void BuildDescriptors();

	void RebuildDescriptors();


	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ComPtr<ID3D12Resource> GetSsaoMap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetSsaoRtv();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSsaoSrv();

private:
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;

	ComPtr<ID3D12Resource> mSsaoMap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhSsaoMapCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhSsaoMapGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhSsaoMapCpuRtv;

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


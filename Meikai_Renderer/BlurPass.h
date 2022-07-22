#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;
class DXApp;
class BlurPass
{
	DXApp* mdxApp = nullptr;
public:
	BlurPass(DXApp* device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3DBlob> computeShader,
		UINT blurwWidth, UINT blurHeight);
	BlurPass(const BlurPass& rhs) = delete;
	BlurPass& operator=(const BlurPass& rhs) = delete;
	~BlurPass() = default;

	static const DXGI_FORMAT blurMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void BuildBlurResource();
	void CreateSrvUavDescriptorHeap();
	void BuildComputePSO();
	void BuildRootSignature();

public:
	void BuildDescriptors();

private:
	ComPtr<ID3D12DescriptorHeap> mSrvUavHeap;

	ComPtr<ID3D12Resource> mBlurHorizon;
	ComPtr<ID3D12Resource> mBlurVertical;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlurHCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlurHCpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlurVCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlurVCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlurHGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlurHGpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlurVGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlurVGpuUav;

	UINT mBlurWidth;
	UINT mBlurHeight;

	ComPtr<ID3DBlob> mComputeShader;

public:
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPso;
};


#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <memory>

#include "UploadBuffer.hpp"
#include "ConstantBuffers.h"

class DXApp;
using namespace Microsoft::WRL;

class LightingPass
{
public:
	LightingPass(DXApp* mApp,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader,
		UINT width, UINT height);
	LightingPass(const LightingPass& rhs) = delete;
	LightingPass& operator=(const LightingPass& rhs) = delete;
	~LightingPass() = default;

	static const DXGI_FORMAT ColorMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void OnResize(UINT newWidth, UINT newHeight);

	void BuildResource();
	void BuildPSO();
	void BuildRootSignature();
	void BuildCbvheap();
	void BuildCbvDesc();

private:
	DXApp* mdxApp = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE* backBufferView;

	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mCbvDesc;

	UINT mRenderTargetWidth;
	UINT mRenderTargetHeight;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	ComPtr<ID3DBlob> mVertShader;
	ComPtr<ID3DBlob> mPixelShader;

public:
	std::unique_ptr<UploadBuffer<LightCB>> mLightCB = nullptr;
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPso;
};


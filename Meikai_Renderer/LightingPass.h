#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;

class LightingPass
{
public:
	LightingPass(ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		UINT width, UINT height);
	LightingPass(const LightingPass& rhs) = delete;
	LightingPass& operator=(const LightingPass& rhs) = delete;
	~LightingPass() = default;

	static const DXGI_FORMAT ColorMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

public:
	void OnResize(UINT newWidth, UINT newHeight);

private:
	ComPtr<ID3D12Device> mdxDevice;
	D3D12_CPU_DESCRIPTOR_HANDLE* backBufferView;

	UINT mRenderTargetWidth;
	UINT mRenderTargetHeight;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

public:
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPso;
};


#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
using Microsoft::WRL::ComPtr;
class DXApp;
struct Texture;
class SkyboxPass
{
public:
	SkyboxPass(DXApp* mApp,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader,
		UINT width, UINT height);

	SkyboxPass(const SkyboxPass& rhs) = delete;
	SkyboxPass& operator=(const SkyboxPass& rhs) = delete;
	~SkyboxPass() = default;

	static const DXGI_FORMAT ColorMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const DXGI_FORMAT mDepthStencilDsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();

private:
	void OnResize(UINT newWidth, UINT newHeight);

	void BuildResource(ComPtr<ID3D12GraphicsCommandList> cmdList);
	void BuildPSO();
	void BuildRootSignature();
	void BuildSrvHeap();
	void BuildSrvDesc();

private:
	DXApp* mdxApp = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE* backBufferView;

	ComPtr<ID3D12DescriptorHeap> mCubeSrvHeap;
	std::unique_ptr<Texture> mCubeTexture;

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


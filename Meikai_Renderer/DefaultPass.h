#pragma once
#include <d3dx12.h>
#include <wrl.h>
#include <memory>

using namespace Microsoft::WRL;
class DXApp;
class Shader;

class DefaultPass
{
	DXApp* mApp;
	
public:
	DefaultPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature();
	void InitPSO();

public:
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPSO;

	ComPtr<ID3DBlob> mVertShader;
	ComPtr<ID3DBlob> mPixelShader;

	DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DepthStenilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};


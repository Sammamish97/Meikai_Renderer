#pragma once
#include <d3dx12.h>
#include <wrl.h>
#include <memory>

using namespace Microsoft::WRL;
class DXApp;
class Shader;

class IPass
{
protected:
	DXApp* mApp;

public:
	IPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	IPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader);
	virtual void InitRootSignature() = 0;
	virtual void InitPSO() = 0;

public:
	ComPtr<ID3D12RootSignature> mRootSig;
	ComPtr<ID3D12PipelineState> mPSO;

	ComPtr<ID3DBlob> mVertShader;
	ComPtr<ID3DBlob> mPixelShader;
	ComPtr<ID3DBlob> mComputeShader;
};


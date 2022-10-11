#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

class ShadowPass : public IPass
{
public:
	ShadowPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};


#pragma once
#include "IPass.h"

enum LightingRootIndex
{

};

class LightingPass : public IPass
{
public:
	LightingPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};


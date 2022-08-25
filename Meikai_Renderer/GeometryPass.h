#pragma once
#include "IPass.h"

class GeometryPass : public IPass
{
public:
	GeometryPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};


#pragma once
#include "IPass.h"

struct LightDescIndices
{
	const UINT TexNum = 6;
	UINT Pos;
	UINT Normal;
	UINT Albedo;
	UINT Roughness;
	UINT Metalic;
	UINT SSAO;
};

class LightingPass : public IPass
{
public:
	LightingPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
	void InitDescIndices();

public:
	LightDescIndices mLightDescIndices;
};


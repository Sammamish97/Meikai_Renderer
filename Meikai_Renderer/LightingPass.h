#pragma once
#include <DirectXMath.h>

#include "IPass.h"

struct LightDescIndices
{
	const UINT TexNum = 9;
	UINT Pos;
	UINT Normal;
	UINT Albedo;
	UINT Roughness;
	UINT Metalic;
	UINT SSAO;
	UINT IBL_DIFFUSE;
	UINT IBL_SPECULAR;
	UINT ShadowDepth;
};

class LightingPass : public IPass
{
public:
	LightingPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader, UINT iblDiffuseIdx, UINT iblSpecularIdx);
	void InitRootSignature() override;
	void InitPSO() override;
	void InitDescIndices();

public:
	LightDescIndices mLightDescIndices;
};


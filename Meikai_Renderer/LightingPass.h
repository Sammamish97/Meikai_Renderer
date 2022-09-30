#pragma once
#include "IPass.h"

enum LightingRootIndex
{
	
};

struct LightingTextureIndices
{
	UINT posIdx = 0;
	UINT normalIdx = 0;
	UINT albedoIdx = 0;
	UINT roughnessIDX = 0;
	UINT metalicIDX = 0;
	UINT aoIdx = 0;
};

class LightingPass : public IPass
{
public:
	LightingPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;

public:
	LightingTextureIndices mTexIndices;
};


#pragma once
#include "IPass.h"
#include "BlurPassIndices.h"
class DXApp;
class Shader;

class BlurPassV : public IPass
{
public:
	BlurPassV(DXApp* appPtr, ComPtr<ID3DBlob> computeShader);
	void InitRootSignature() override;
	void InitPSO() override;

public:
	BlurPassIndices mBlurDescIndices;
};


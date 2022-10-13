#pragma once
#include "IPass.h"
#include "BlurPassIndices.h"
class DXApp;
class Shader;


class BlurPass : public IPass
{
public:
	BlurPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader);
	void InitRootSignature() override;
	void InitPSO() override;
	std::vector<float> CalcGaussWeights(float sigma);

public:
	BlurPassIndices mBlurDescIndices;
};


#pragma once
#include "IPass.h"

struct SsaoIndices
{
	const UINT TexNum = 3;
	UINT Pos;
	UINT Normal;
	UINT Depth;
};


using namespace Microsoft::WRL;
class DXApp;
class Shader;

class SsaoPass : public IPass
{
public:
	SsaoPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;

public:
	SsaoIndices mSsaoIndices;
};




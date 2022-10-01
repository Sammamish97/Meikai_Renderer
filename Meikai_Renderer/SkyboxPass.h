#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

struct SkyboxDescIndices
{
	const UINT TexNum = 1;
	UINT Skybox;
};

class SkyboxPass : public IPass
{
public:
	SkyboxPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader, UINT skyboxSrvIdx);
	void InitRootSignature() override;
	void InitPSO() override;

public:
	SkyboxDescIndices mSkyboxDescIndices;
};


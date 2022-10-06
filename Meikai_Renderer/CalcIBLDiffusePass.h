#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

struct IBLDiffuseIndices
{
	const UINT TexNum = 2;
	UINT HDR_SRV_CUBE;
	UINT UAV_IBL_DIFFUSE_CUBE;
};

struct IBLDiffuseCB
{
	// Size of the cubemap face in pixels at the current mipmap level.
	// Currently use only mipmap 0.
	uint32_t CubemapSize;
};

class CalcIBLDiffusePass : public IPass
{
public:
	CalcIBLDiffusePass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader, UINT hdrCubeSrvIdx, UINT IBLDiffuseCubeUrvIdx);
	void InitRootSignature() override;
	void InitPSO() override;

public:
	IBLDiffuseIndices mDiffuseDescIndices;
};


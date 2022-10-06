#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

struct EquiRectDescIndices
{
	const UINT TexNum = 3;
	UINT HDR_SRV_2D;
	UINT Cubemap_UAV_Skybox;
	UINT Cubemap_UAV_HDR;
};

struct EquiRectToCubemapCB
{
    // Size of the cubemap face in pixels at the current mipmap level.
	// Currently use only mipmap 0.
    uint32_t CubemapSize;
};

class EquiRectToCubemapPass : public IPass
{
public:
	EquiRectToCubemapPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader, UINT hdrSrvIdx, UINT SkyboxCubemapUrvIdx, UINT CubemapUrvIdx);
	void InitRootSignature() override;
	void InitPSO() override;

public:
	EquiRectDescIndices mEquiRectDescIndices;
};


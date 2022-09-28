#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

enum EquiPassRootIndex
{

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
	EquiRectToCubemapPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader);
	void InitRootSignature() override;
	void InitPSO() override;
};


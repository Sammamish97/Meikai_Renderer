#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

class DebugMeshPass : public IPass
{
public:
	DebugMeshPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};
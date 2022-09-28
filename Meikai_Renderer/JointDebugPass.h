#pragma once
#include "IPass.h"

using namespace Microsoft::WRL;
class DXApp;
class Shader;

enum JointRootIndex
{

};


class JointDebugPass : public IPass
{
public:
	JointDebugPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};
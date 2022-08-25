#include "IPass.h"
#include "DXApp.h"

IPass::IPass(DXApp* appPtr, ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:mApp(appPtr), mVertShader(vertShader), mPixelShader(pixelShader), mComputeShader(nullptr)
{

}
IPass::IPass(DXApp* appPtr, ComPtr<ID3DBlob> computeShader)
	:mApp(appPtr), mVertShader(nullptr), mPixelShader(nullptr), mComputeShader(computeShader)
{

}
#include "LightingPass.h"

LightingPass::LightingPass(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList, UINT width,
	UINT height)
	:mdxDevice(device)
{
	OnResize(width, height);
}

void LightingPass::OnResize(UINT newWidth, UINT newHeight)
{
    if (mRenderTargetWidth != newWidth || mRenderTargetHeight != newHeight)
    {
        mRenderTargetWidth = newWidth;
        mRenderTargetHeight = newHeight;

        // We render to ambient map at half the resolution.
        mViewport.TopLeftX = 0.0f;
        mViewport.TopLeftY = 0.0f;
        mViewport.Width = mRenderTargetWidth / 2.0f;
        mViewport.Height = mRenderTargetHeight / 2.0f;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;

        mScissorRect = { 0, 0, (int)mRenderTargetWidth / 2, (int)mRenderTargetHeight / 2 };
    }
}

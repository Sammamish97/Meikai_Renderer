#pragma once
#include <DirectXMath.h>
#include "MathHelper.h"

using namespace DirectX;

struct ObjectCB
{
    XMFLOAT4X4 World = MathHelper::Identity4x4();
};

struct CommonCB
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
    XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    XMFLOAT4X4 ViewProjTex = MathHelper::Identity4x4();
    XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
};

struct DirectLight
{
    XMFLOAT3 Direction;
    float padding1;

    XMFLOAT3 Color;
    float padding2;
};

struct PointLight
{
    XMFLOAT3 Position;
    float padding1;

    XMFLOAT3 Color;
    float padding2;
};

struct LightCB
{
    DirectLight directLight;
    PointLight pointLight[3];
};

struct RandomSampleCB
{
    RandomSampleCB()
    {
        int kk;
        for (int k = 0; k < N; k++)
        {
            float u = 0;
            float p = 0.5f;
            for (kk = k, u = 0.0f; kk; p *= 0.5f, kk >>= 1)
            {
                if (kk & 1)
                    u += p;
            }
            float v = (k + 0.5) / N;
            hammersley[k] = XMFLOAT4(u, v, 0, 0);
        }
    }

    float N = 20.f;
    XMFLOAT3 padding;
    XMFLOAT4 hammersley[20];
};

struct ShadowCB
{
    XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
};

struct GeometryCB
{

};

struct SsaoCB
{
	
};

struct PostProcessCB
{
	
};
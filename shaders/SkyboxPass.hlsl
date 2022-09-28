cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gViewProjTex;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

TextureCube gCubeMap : register(t8);

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : UV;
	float3 TangentU : TANGENT;
    float3 BiTangentU : BITANGENT;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 CubemapUV : UV;
};

VertexOut VS(VertexIn vin)
{
    float4x4 posRemoveView = gView;

    posRemoveView._m30 = 0;
    posRemoveView._m31 = 0;
    posRemoveView._m32 = 0;
    posRemoveView._m33 = 0;
    
    VertexOut output;
    output.CubemapUV = vin.PosL;
    output.PosH = mul(float4(vin.PosL, 1.0), posRemoveView);
    output.PosH = mul(output.PosH, gProj).xyww;
    return output;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gsamLinearWrap, pin.CubemapUV);
}

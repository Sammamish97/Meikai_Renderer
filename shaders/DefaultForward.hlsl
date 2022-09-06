struct WorldMatrix
{
    matrix mat;
};

ConstantBuffer<WorldMatrix> objectWorld : register(b0);
Texture2D<float4> testTexture : register(t6);


cbuffer cbPass : register(b1)
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

float3 ToneMapping(float3 HDRColor)
{
    const float gamma = 2.2;
    float3 mapped = HDRColor / (HDRColor + float3(1.f, 1.f, 1.f));
    return pow(mapped, float3(1.f, 1.f, 1.f) / gamma);
}

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
	float4 position     : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), objectWorld.mat);
    vout.position = mul(posW, gViewProj);
    vout.normal = vin.NormalL;
    vout.uv = vin.TexC;

    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
    float3 texColor = testTexture.SampleLevel(gsamPointClamp, pin.uv, 0.0f).xyz;
    //여기 HDR 톤매핑 기능을 넣어야할듯
	// Interpolating normal can unnormalize it, so renormalize it.
    PS_OUTPUT output;
    output.Color = float4(ToneMapping(texColor), 1);
    return output;
}

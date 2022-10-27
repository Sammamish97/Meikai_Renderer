#include "PBRCalc.hlsli"

#define MAX_UV 20
struct DirectLight
{
	float3 direct;
	float padding1;

	float3 color;
	float padding2;
};

struct PointLight
{
	float3 position;
	float padding1;

	float3 color;
	float padding2;
};

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

cbuffer lightData : register(b1)
{
	DirectLight dirLight;
	PointLight pointLights[3];
};

struct DescIndices
{
	uint TexNum;
	uint Pos;
	uint Normal;
	uint Albedo;
	uint Roughness;
	uint Metalic;
	uint SSAO;
	uint IBL_DIFFUSE;
	uint IBL_SPECULAR;
	uint ShadowDepth;
};

ConstantBuffer<DescIndices> srvIndices : register(b2);

struct RandomSampling
{
	float sampleNumber;
	float3 padding;
	float4 sampledUV[MAX_UV];
	int4 HDRTexDim;
};

cbuffer randomData : register(b3)
{
	RandomSampling randomValues;
};

struct Matrix
{
    matrix mat;
};
ConstantBuffer<Matrix> shadowVP : register(b4);


Texture2D<float4> gTable[] : register(t0, space0);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);
SamplerState gsamLinearRepeat : register(s4);
SamplerComparisonState gShadowSampler : register(s5);

float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;
    uint width, height, numMips;
    gTable[srvIndices.ShadowDepth].GetDimensions(0, width, height, numMips);
	
    // Texel size.
    float dx = 1.0f / (float)width;
	float dy = 1.0f / (float)height;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dy), float2(0.0f,  -dy), float2(dx,  -dy),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dy), float2(0.0f,  +dy), float2(dx,  +dy)
    };

    [unroll]
    for(int i = 0; i < 9; ++i)
    {
        percentLit += gTable[srvIndices.ShadowDepth].SampleCmpLevelZero(gShadowSampler,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

struct VertexOut
{
    float4 posH : SV_POSITION;
	float2 UV : TEXCOORD;
};
// ----------------------------------------------------------------------------
float4 PS(VertexOut pin) : SV_Target
{
	float2 fliped_UV = pin.UV;
	fliped_UV.y = 1 - fliped_UV.y;

	float3 position = gTable[srvIndices.Pos].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float3 normal = normalize(gTable[srvIndices.Normal].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz);
	float3 albedo = gTable[srvIndices.Albedo].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float roughness = gTable[srvIndices.Roughness].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).x;
	float metalic = gTable[srvIndices.Metalic].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).x;
	float occluded = gTable[srvIndices.SSAO].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).x;

	float4 ShadowPos = mul(float4(position, 1.0f), shadowVP.mat);

	float3 N = normalize(normal);
	float3 V = normalize(gEyePosW - position);

	float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, albedo, metalic);
	float3 LightOutput = float3(0, 0, 0);
	
	for(int i = 0; i < 3; ++i)
	{
		LightOutput += LightEquation(V, N, position, 
		pointLights[i].position, pointLights[i].color, albedo, roughness, metalic);
	}
	float2 normal_UV = VecToUv(normal);

	float3 IBL_Diffuse = gTable[srvIndices.IBL_DIFFUSE].SampleLevel(gsamLinearRepeat, normal_UV, 0.0f).xyz;
	
	float3 ambient_diffuse = IBL_Diffuse * albedo / PI;
	float3 ambient_specular = float3(0, 0, 0);

	float3 reflected = 2 * dot(N, V) * N - V;
	float3 A = normalize(float3(reflected.z, 0, -reflected.x));
	float3 B = normalize(cross(reflected, A));
	for(int i = 0; i < randomValues.sampleNumber; ++i)
	{
		float3 zOrientSample = SampleRandomVectorGGX(randomValues.sampledUV[i].x,
			randomValues.sampledUV[i].y,
			roughness);
		float3 w_i = RotateZaxisToLightAxis(zOrientSample, reflected, A, B);
		float distribution = DistributionGGX(N, normalize(V + w_i), roughness);

		float level = (0.5 * log2(randomValues.HDRTexDim.x * randomValues.HDRTexDim.y / randomValues.sampleNumber)) - (0.5 * log2(distribution));

		level = (roughness == 0) ? 0.0 : level;
		float3 sampledColor = gTable[srvIndices.IBL_SPECULAR].SampleLevel(gsamLinearRepeat, VecToUv(w_i), level).xyz;

		ambient_specular += IBLSpecular(V, N, w_i, sampledColor, albedo, roughness, metalic);
	}
	ambient_specular /= randomValues.sampleNumber;

	float shadowFactor = CalcShadowFactor(ShadowPos);
	float3 resultHDR = ambient_diffuse + ambient_specular + LightOutput;
	float3 resultLDR = ToneMapping(resultHDR, 5) * shadowFactor;
    return float4(resultLDR, 1.0);
	//return float4(occluded, occluded, occluded, 1);
}
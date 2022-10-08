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

struct RandomSampling
{
	float sampleNumber;
	float3 padding;
	float4 sampledUV[MAX_UV];
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
};

ConstantBuffer<DescIndices> srvIndices : register(b2);

cbuffer randomData : register(b3)
{
	RandomSampling randomValues;
};

Texture2D<float4> gTable[] : register(t0, space0);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

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

	float3 N = normal;
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

	float3 IBL_Diffuse = gTable[srvIndices.IBL_DIFFUSE].SampleLevel(gsamPointClamp, normal_UV, 0.0f).xyz;
	
	float3 ambient_diffuse = IBL_Diffuse * albedo / PI;
	float3 ambient_specular = float3(0, 0, 0);

	float3 reflected = 2 * dot(N, V) * N - V;
	for(int i = 0; i < randomValues.sampleNumber; ++i)
	{
		float3 zOrientSample = SampleRandomVectorGGX(randomValues.sampledUV[i].x,
			randomValues.sampledUV[i].y,
			roughness);
		float3 w_i = RotateZaxisToLightAxis(zOrientSample, reflected);
		float3 sampledColor = gTable[srvIndices.IBL_SPECULAR].SampleLevel(gsamPointClamp, VecToUv(w_i), 0.0f).xyz;

		float3 H = normalize(V + w_i);
		float NdotL = max(dot(N, w_i), 0.0);

		float G = GeometrySmith(N, V, w_i, roughness);
		float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		float3 nom = G * F;
		float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, w_i), 0.0) + 0.0001;
		
		float3 result = sampledColor * NdotL * (nom / denom);
		ambient_specular += result;
	}
	ambient_specular /= randomValues.sampleNumber;

	float3 resultHDR = ambient_specular;
	float3 resultLDR = ToneMapping(resultHDR, 5);
    return float4(resultLDR, 1.0);
}
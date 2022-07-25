Texture2D gPositionMap : register(t0);
Texture2D gNormalMap  : register(t1);
Texture2D gAlbedoMap  : register(t2);
Texture2D gDepthMap  : register(t3);

Texture2D gSsaoMap  : register(t4);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

// Constant data that varies per material.

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

cbuffer lightData : register(b2)
{
	DirectLight dirLight;
	PointLight pointLights[3];
};

struct VertexOut
{
    float4 posH : SV_POSITION;
	float2 UV : TEXCOORD;
};

float4 PS(VertexOut pin) : SV_Target
{
	float2 fliped_UV = pin.UV;
	fliped_UV.y = 1 - fliped_UV.y;

	float3 position = gPositionMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float3 normal = normalize(gNormalMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz);
	float3 albedo = gAlbedoMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float3 occluded = gSsaoMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;

	float3 resultColor = float3(0, 0, 0);
	
	float3 normalizedDir = normalize(dirLight.direct);
	resultColor += max(dot(normalizedDir, normal), 0.0f) * dirLight.color;

	for(int i = 0; i < 3; ++i)
	{
		normalizedDir = normalize(pointLights[i].position - position);
		resultColor += max(dot(normalizedDir, normal), 0.0f) * pointLights[i].color;
	}
	resultColor *= occluded;
    return float4(resultColor, 1.0);
}
 
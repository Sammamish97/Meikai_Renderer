Texture2D gPositionMap : register(t0);
Texture2D gNormalMap  : register(t1);
Texture2D gAlbedoMap  : register(t2);
Texture2D gRoughness  : register(t3);
Texture2D gMetalic  : register(t4);
Texture2D gSsaoMap  : register(t5);
Texture2D gDepthMap  : register(t6);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

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

struct VertexOut
{
    float4 posH : SV_POSITION;
	float2 UV : TEXCOORD;
};
static const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------



float4 PS(VertexOut pin) : SV_Target
{
	float2 fliped_UV = pin.UV;
	fliped_UV.y = 1 - fliped_UV.y;

	float3 position = gPositionMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float3 normal = normalize(gNormalMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz);
	float3 albedo = gAlbedoMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float occluded = gSsaoMap.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).x;
	float metalic = gMetalic.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).x;
	float roughness = gRoughness.SampleLevel(gsamPointClamp, fliped_UV, 0.0f).x;

	float3 N = normal;
	float3 V = normalize(gEyePosW - position);

	float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, albedo, metalic);
	float3 LightOutput = float3(0, 0, 0);
	
	for(int i = 0; i < 3; ++i)
	{
		float3 L = normalize(pointLights[i].position - position);
		float3 H = normalize(V + L);
		float distance = length(pointLights[i].position - position);
		float attenuation = 1.0 / (distance * distance);
		float3 radiance = pointLights[i].color * attenuation;

		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		float3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		float3 specular = numerator / denominator;

		float3 KS = F;
		float3 KD = float3(1.0, 1.0, 1.0) - KS;
		KD *= 1.0 - metalic;

		float NdotL = max(dot(N, L), 0.0);

		LightOutput += (KD * albedo / PI + specular) * radiance * NdotL;
	}

	float3 ambient = float3(0.03, 0.03, 0.03) * albedo * occluded;
	
	float3 resultColor = ambient + LightOutput;
    return float4(resultColor, 1.0);
}
 
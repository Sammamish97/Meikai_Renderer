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

float3 UvToVec(float u, float v)
{
	float x = cos(2.0 * PI * (0.5 - u)) * sin(PI * v);
	float y = sin(2.0 * PI * (0.5 - u)) * sin(PI * v);
	float z = cos(PI * v);
	return float3(x, y, z);
}
// ----------------------------------------------------------------------------

float2 VecToUv(float3 dir)
{
	float u = 0.5 - atan2(dir.y, dir.x) / (2 * PI);
	float v = acos(dir.z) / PI;
	return float2(u, v);
}
// ----------------------------------------------------------------------------

float3 SampleRandomVectorGGX(float u, float v, float roughness)
{
	float nom = roughness * sqrt(v);
	float dnom = sqrt(1 - v);
	float theta = atan(nom/dnom);
	float2 newUV = float2(u, theta);
	return normalize(UvToVec(newUV.x, newUV.y / PI));
}

// ----------------------------------------------------------------------------
float3 RotateZaxisToLightAxis(float3 zSampled, float3 reflected)
{
	reflected = normalize(reflected);
	float3 A = normalize(float3(-reflected.y, reflected.x, 0));
	float3 B = normalize(cross(reflected, A));
	return normalize(zSampled.x * A + zSampled.y * B + zSampled.z * reflected);
}

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

		float3 H = normalize(V + w_i);
		float NdotL = max(dot(N, w_i), 0.0);

		float G = GeometrySmith(N, V, w_i, roughness);
		float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		//float3 numerator = G * F;
		float3 numerator = float3(0.5, 0.5, 0.5);
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, w_i), 0.0) + 0.0001;
		float3 IBL_Specular = gTable[srvIndices.IBL_SPECULAR].SampleLevel(gsamPointClamp, VecToUv(w_i), 0.0f).xyz;

		ambient_specular += numerator / denominator * IBL_Specular * NdotL;
	}
	ambient_specular /= randomValues.sampleNumber;
	//float3 resultColor = ambient_diffuse + ambient_specular;
	float3 resultColor = ambient_specular;
    return float4(resultColor, 1.0);
}
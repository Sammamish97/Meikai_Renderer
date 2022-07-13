Texture2D gPositionMap : register(t0);
Texture2D gNormalMap  : register(t1);
Texture2D gAlbedoMap  : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

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

    return float4(position, 1.0);
}

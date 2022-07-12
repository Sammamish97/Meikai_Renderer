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
    float4 posH : SV_POSITION;
	float2 UV : TEXCOORD;
};

Texture2D gPositionMap : register(t0);
Texture2D gNormalMap  : register(t1);
Texture2D gAlbedoMap  : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	vout.posH = float4(vin.PosL, 1.f);
	vout.UV = vin.TexC;
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float3 position = gPositionMap.SampleLevel(gsamPointClamp, pin.UV, 0.0f).xyz;
	float3 normal = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.UV, 0.0f).xyz);
	float3 albedo = gAlbedoMap.SampleLevel(gsamPointClamp, pin.UV, 0.0f).xyz;

    return float4(position, 1.0);
}

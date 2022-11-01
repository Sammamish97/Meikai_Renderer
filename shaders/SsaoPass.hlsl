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

struct DescIndices
{
	uint TexNum;
	uint Pos;
	uint Normal;
	uint Depth;
};
ConstantBuffer<DescIndices> srvIndices : register(b1);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);
SamplerState gsamLinearRepeat : register(s4);

Texture2D<float4> gTable[] : register(t0, space0);

struct VertexOut
{
    float4 posH : SV_POSITION;
	float2 UV : TEXCOORD;
};

float4 PS(VertexOut pin) : SV_Target
{
	float2 fliped_UV = pin.UV;
	fliped_UV.y = 1 - fliped_UV.y;

	float3 worldPosition = gTable[srvIndices.Pos].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz;
	float3 normal = normalize(gTable[srvIndices.Normal].SampleLevel(gsamPointClamp, fliped_UV, 0.0f).xyz);
	float depth = gTable[srvIndices.Depth].SampleLevel(gsamDepthMap, fliped_UV, 0.0f).r;

	int2 pixelScreenCoord = pin.posH.xy;
	float2 pixelNDCCoord = pixelScreenCoord / gRenderTargetSize;

	int sampleCount = 50;
	float radius = 0.2;
	float phi = (30 * pixelScreenCoord.x ^ pixelScreenCoord.y) + 10 * pixelScreenCoord.x * pixelScreenCoord.y;
	float fallOff = radius * 0.1;
	float depth_threshold = 0.001;

	float accumulation = 0;

	for(int i = 0; i < sampleCount; ++i)
	{
		float alpha = (i + 0.5) / sampleCount;
		float height = alpha * radius / depth;
		float theta = 2.0 * 3.14 * alpha * (7.0 * sampleCount / 9.0) + phi;

		float2 sampledUV = pixelNDCCoord + height * float2(cos(theta), sin(theta));
		float3 sampledPos = gTable[srvIndices.Pos].SampleLevel(gsamPointClamp, sampledUV, 0.0f).xyz;

		float3 origin_to_sample = sampledPos - worldPosition;
		float3 w_i = normalize(origin_to_sample);

		float occlusion_amount = max(0, dot(normal, w_i) - depth_threshold * depth);
		float visibility = (radius - length(origin_to_sample)) > 0 ? 1 : 0;
		float denominator = max(fallOff * fallOff, dot(w_i, w_i));

		accumulation += occlusion_amount * visibility / denominator;

	}

	float total_occluded = 2 * 3.14 * fallOff / sampleCount * accumulation;
	total_occluded = max(0, total_occluded);

    float result = pow((1 - total_occluded), 100);
	return float4(result, result, result, 1.0);
}

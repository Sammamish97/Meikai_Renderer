Texture2D gPositionMap : register(t0);
Texture2D gNormalMap  : register(t1);
Texture2D gAlbedoMap  : register(t2);
Texture2D gDepthMap  : register(t3);

Texture2D gInput            : register(t4);

RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N + 2*5)
groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	float4 blurColor = float4(0, 0, 1, 1);
	gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	float4 blurColor = float4(1, 0, 0, 1);
	gOutput[dispatchThreadID.xy] = blurColor;
}
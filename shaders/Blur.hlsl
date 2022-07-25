cbuffer cbSettings : register(b0)
{
	// We cannot have an array entry in a constant buffer that gets mapped onto
	// root constants, so list each element.  
	
	int gBlurRadius;

	// Support up to 11 blur weights.
	float w0;
	float w1;
	float w2;
	float w3;
	float w4;
	float w5;
	float w6;
	float w7;
	float w8;
	float w9;
	float w10;
};

Texture2D gPositionMap : register(t0);
Texture2D gNormalMap  : register(t1);
Texture2D gAlbedoMap  : register(t2);
Texture2D gDepthMap  : register(t3);

Texture2D gInput            : register(t4);

RWTexture2D<float4> gOutput : register(u0);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

#define N 256
static const int gMaxBlurRadius = 5;

#define N 256
#define CacheSize (N + 2 * gMaxBlurRadius)
groupshared float4 gInputCache[CacheSize];
groupshared float4 gNormalCache[CacheSize];
groupshared float4 gDepthCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
	float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	if(groupThreadID.x < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gInputCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
		gNormalCache[groupThreadID.x] = gNormalMap[int2(x, dispatchThreadID.y)];
		gDepthCache[groupThreadID.x] = gDepthMap[int2(x, dispatchThreadID.y)];
	}
	if(groupThreadID.x >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x-1);
		gInputCache[groupThreadID.x+2*gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
		gNormalCache[groupThreadID.x+2*gBlurRadius] = gNormalMap[int2(x, dispatchThreadID.y)];
		gDepthCache[groupThreadID.x+2*gBlurRadius] = gDepthMap[int2(x, dispatchThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
	gInputCache[groupThreadID.x+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];
	gNormalCache[groupThreadID.x+gBlurRadius] = gNormalMap[min(dispatchThreadID.xy, gInput.Length.xy-1)];
	gDepthCache[groupThreadID.x+gBlurRadius] = gDepthMap[min(dispatchThreadID.xy, gInput.Length.xy-1)];
	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = float4(0, 0, 0, 0);
	
	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		int k = groupThreadID.x + gBlurRadius + i;
		
		blurColor += weights[i + gBlurRadius] * gInputCache[k];
	}
	
	gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
	float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	if(groupThreadID.y < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = max(dispatchThreadID.y - gBlurRadius, 0);
		gInputCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
		gNormalCache[groupThreadID.y] = gNormalMap[int2(dispatchThreadID.x, y)];
		gDepthCache[groupThreadID.y] = gDepthMap[int2(dispatchThreadID.x, y)];
	}
	if(groupThreadID.y >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y-1);
		gInputCache[groupThreadID.y+2*gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
		gNormalCache[groupThreadID.y + 2 * gBlurRadius] = gNormalMap[int2(dispatchThreadID.x, y)];
		gDepthCache[groupThreadID.y + 2 * gBlurRadius] = gDepthMap[int2(dispatchThreadID.x, y)];
	}
	
	// Clamp out of bound samples that occur at image borders.
	gInputCache[groupThreadID.y+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];
	gNormalCache[groupThreadID.y + gBlurRadius] = gNormalMap[min(dispatchThreadID.xy, gInput.Length.xy - 1)];
	gDepthCache[groupThreadID.y + gBlurRadius] = gDepthMap[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = float4(0, 0, 0, 0);
	
	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		int k = groupThreadID.y + gBlurRadius + i;
		
		blurColor += weights[i + gBlurRadius] * gInputCache[k];
	}
	
	gOutput[dispatchThreadID.xy] = blurColor;
}
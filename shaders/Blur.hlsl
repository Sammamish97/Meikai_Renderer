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

struct DescIndices
{
	uint TexNum;
	uint Normal;
	uint Depth;
	uint SRVInput;
	uint UAVOutput;
};

ConstantBuffer<DescIndices> descIndices : register(b1);

Texture2D<float4> gSrvTable[] : register(t0, space0);
RWTexture2D<float4> gUavTable[] : register(u0, space0);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);
SamplerState gsamLinearRepeat : register(s4);

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
		gInputCache[groupThreadID.x] = gSrvTable[descIndices.SRVInput][int2(x, dispatchThreadID.y)];
		gNormalCache[groupThreadID.x] = gSrvTable[descIndices.Normal][int2(x, dispatchThreadID.y)];
		gDepthCache[groupThreadID.x] = gSrvTable[descIndices.Depth][int2(x, dispatchThreadID.y)];
	}
	if(groupThreadID.x >= N - gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(dispatchThreadID.x + gBlurRadius, gSrvTable[descIndices.SRVInput].Length.x-1);
		gInputCache[groupThreadID.x+2*gBlurRadius] = gSrvTable[descIndices.SRVInput][int2(x, dispatchThreadID.y)];
		gNormalCache[groupThreadID.x+2*gBlurRadius] = gSrvTable[descIndices.Normal][int2(x, dispatchThreadID.y)];
		gDepthCache[groupThreadID.x+2*gBlurRadius] = gSrvTable[descIndices.Depth][int2(x, dispatchThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
	gInputCache[groupThreadID.x+gBlurRadius] = gSrvTable[descIndices.SRVInput][min(dispatchThreadID.xy, gSrvTable[descIndices.SRVInput].Length.xy-1)];
	gNormalCache[groupThreadID.x+gBlurRadius] = gSrvTable[descIndices.Normal][min(dispatchThreadID.xy, gSrvTable[descIndices.SRVInput].Length.xy-1)];
	gDepthCache[groupThreadID.x+gBlurRadius] = gSrvTable[descIndices.Depth][min(dispatchThreadID.xy, gSrvTable[descIndices.SRVInput].Length.xy-1)];
	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = weights[gBlurRadius] * gInputCache[groupThreadID.x];
	float totalWeight = weights[gBlurRadius];

	float3 centerNormal = gNormalCache[groupThreadID.x];
	float centerDepth = gDepthCache[groupThreadID.x];

	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		if(i == 0) continue;
		int k = groupThreadID.x + gBlurRadius + i;

		float3 neighborNormal =  gNormalCache[k];
		float neighborDepth = gDepthCache[k];

		if(dot(neighborNormal, centerNormal) >= 0.99f && abs(neighborDepth - centerDepth) <= 0.01f)
		{
			float weight = weights[i + gBlurRadius];
			blurColor += weight * gInputCache[k];
			totalWeight += weight;
		}
		// float weight = weights[i + gBlurRadius];
		// blurColor += weight * gInputCache[k];
		// totalWeight += weight;
	}
	gUavTable[descIndices.UAVOutput][dispatchThreadID.xy - 0.5] = blurColor / totalWeight;
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
		gInputCache[groupThreadID.y] = gSrvTable[descIndices.SRVInput][int2(dispatchThreadID.x, y)];
		gNormalCache[groupThreadID.y] = gSrvTable[descIndices.Normal][int2(dispatchThreadID.x, y)];
		gDepthCache[groupThreadID.y] = gSrvTable[descIndices.Depth][int2(dispatchThreadID.x, y)];
	}
	if(groupThreadID.y >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = min(dispatchThreadID.y + gBlurRadius, gSrvTable[descIndices.SRVInput].Length.y-1);
		gInputCache[groupThreadID.y + 2 * gBlurRadius] = gSrvTable[descIndices.SRVInput][int2(dispatchThreadID.x, y)];
		gNormalCache[groupThreadID.y + 2 * gBlurRadius] = gSrvTable[descIndices.Normal][int2(dispatchThreadID.x, y)];
		gDepthCache[groupThreadID.y + 2 * gBlurRadius] = gSrvTable[descIndices.Depth][int2(dispatchThreadID.x, y)];
	}
	
	// Clamp out of bound samples that occur at image borders.
	gInputCache[groupThreadID.y + gBlurRadius] = gSrvTable[descIndices.SRVInput][min(dispatchThreadID.xy, gSrvTable[descIndices.SRVInput].Length.xy-1)];
	gNormalCache[groupThreadID.y + gBlurRadius] = gSrvTable[descIndices.Normal][min(dispatchThreadID.xy, gSrvTable[descIndices.SRVInput].Length.xy - 1)];
	gDepthCache[groupThreadID.y + gBlurRadius] = gSrvTable[descIndices.Depth][min(dispatchThreadID.xy, gSrvTable[descIndices.SRVInput].Length.xy - 1)];

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = weights[gBlurRadius] * gInputCache[groupThreadID.y];
	float totalWeight = weights[gBlurRadius];

	float3 centerNormal = gNormalCache[groupThreadID.y];
	float centerDepth = gDepthCache[groupThreadID.y];

	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		if(i == 0) continue;
		int k = groupThreadID.y + gBlurRadius + i;

		float3 neighborNormal =  gNormalCache[k];
		float neighborDepth = gDepthCache[k];

		if(dot(neighborNormal, centerNormal) >= 0.99f && abs(neighborDepth - centerDepth) <= 0.01f)
		{
			float weight = weights[i + gBlurRadius];
			blurColor += weight * gInputCache[k];
			totalWeight += weight;
		}
		// float weight = weights[i + gBlurRadius];
		// blurColor += weight * gInputCache[k];
		// totalWeight += weight;
	}
	gUavTable[descIndices.UAVOutput][dispatchThreadID.xy - 0.5] = blurColor / totalWeight;
}
#define BLOCK_SIZE 16

struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct IBLDiffuse
{
    // Size of the cubemap face in pixels at the current mipmap level.
	// Level 0
    uint CubemapSize;
};
ConstantBuffer<IBLDiffuse> IBLDiffuseCB : register(b0);

struct DescIndices
{
	uint TexNum;
	uint HDR_SRV_CUBE;
	uint UAV_IBL_DIFFUSE_CUBE;
};
ConstantBuffer<DescIndices> descIndices : register(b1);

TextureCube gSRVCubeTable[] : register(t0, space0);
RWTexture2DArray<float4> gUAV2DArrayTable[] : register(u0, space0);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);
SamplerState gsamLinearRepeat : register(s4);


// 1 / PI
static const float PI = 3.14159265359;
static const float InvPI = 0.31830988618379067153776752674503f;
static const float Inv2PI = 0.15915494309189533576888376337251f;
static const float2 InvAtan = float2(Inv2PI, InvPI);

// Transform from dispatch ID to cubemap face direction
static const float3x3 RotateUV[6] = {
    // +X
    float3x3(  0,  0,  1,
               0, -1,  0,
              -1,  0,  0 ),
    // -X
    float3x3(  0,  0, -1,
               0, -1,  0,
               1,  0,  0 ),
    // +Y
    float3x3(  1,  0,  0,
               0,  0,  1,
               0,  1,  0 ),
    // -Y
    float3x3(  1,  0,  0,
               0,  0, -1,
               0, -1,  0 ),
    // +Z
    float3x3(  1,  0,  0,
               0, -1,  0,
               0,  0,  1 ),
    // -Z
    float3x3( -1,  0,  0,
               0, -1,  0,
               0,  0, -1 )
};
groupshared float4x4 gCache[65536];
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CalcIBLDiffuseCS(ComputeShaderInput IN)
{
	// Cubemap texture coords.
    uint3 texCoord = IN.DispatchThreadID;

    // First check if the thread is in the cubemap dimensions.
    if (texCoord.x >= IBLDiffuseCB.CubemapSize || texCoord.y >= IBLDiffuseCB.CubemapSize) return;

    // Map the UV coords of the cubemap face to a direction
    // [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
    float3 dir = float3( texCoord.xy / float(IBLDiffuseCB.CubemapSize) - 0.5f, 0.5f);

    // Rotate to cubemap face
    dir = normalize( mul( RotateUV[texCoord.z], dir ) );

    float3 irradiance = float3(0.0, 0.0, 0.0);   

    // tangent space calculation from origin point
    float3 up    = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, dir));
    up = normalize(cross(dir, right));
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * dir;
            irradiance += gSRVCubeTable[descIndices.HDR_SRV_CUBE].SampleLevel(gsamLinearRepeat, sampleVec, 0).xyz * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    gUAV2DArrayTable[descIndices.UAV_IBL_DIFFUSE_CUBE][texCoord] = float4(irradiance, 1);
}
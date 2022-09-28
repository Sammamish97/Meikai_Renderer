#define BLOCK_SIZE 16

struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct PanoToCubemap
{
    // Size of the cubemap face in pixels at the current mipmap level.
	// Level 0
    uint CubemapSize;
};
ConstantBuffer<PanoToCubemap> PanoToCubemapCB : register(b0);

Texture2D gInputHDR : register(t7);
RWTexture2DArray<float4> outputCubemap : register(u0);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

// 1 / PI
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

float3 ToneMapping(float3 HDRColor)
{
    const float gamma = 2.2;
    float3 mapped = HDRColor / (HDRColor + float3(1.f, 1.f, 1.f));
    return pow(mapped, float3(1.f, 1.f, 1.f) / gamma);
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void EquiRectToCubemapCS(ComputeShaderInput IN)
{
	// Cubemap texture coords.
    uint3 texCoord = IN.DispatchThreadID;

    // First check if the thread is in the cubemap dimensions.
    if (texCoord.x >= PanoToCubemapCB.CubemapSize || texCoord.y >= PanoToCubemapCB.CubemapSize) return;

    // Map the UV coords of the cubemap face to a direction
    // [(0, 0), (1, 1)] => [(-0.5, -0.5), (0.5, 0.5)]
    float3 dir = float3( texCoord.xy / float(PanoToCubemapCB.CubemapSize) - 0.5f, 0.5f);

    // Rotate to cubemap face
    dir = normalize( mul( RotateUV[texCoord.z], dir ) );

    // Convert the world space direction into U,V texture coordinates in the panoramic texture.
    // Source: http://gl.ict.usc.edu/Data/HighResProbes/
    float2 panoUV = float2(atan2(-dir.x, -dir.z), acos(dir.y)) * InvAtan;
    float3 HDRColor = gInputHDR.SampleLevel(gsamLinearClamp, panoUV, 0).xyz;

    outputCubemap[texCoord] =  float4(ToneMapping(HDRColor), 1);
}
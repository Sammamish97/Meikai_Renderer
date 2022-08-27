struct WorldMatrix
{
    matrix mat;
};

ConstantBuffer<WorldMatrix> objectWorld : register(b0);

cbuffer cbPass : register(b1)
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

ByteAddressBuffer BufferTable[] : register(t0)

struct MyVertexStructure
{
	float3 Pos;
    float3 Normal;
	float2 UV;
	float3 BiTangent
};

cbuffer MyPerDrawConstants
{
    uint vertexOffset;
    uint vertexBufferIndex;
}

struct VertexOut
{
	float4 Pos     : SV_POSITION;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), objectWorld.mat);
    vout.Pos = mul(posW, gViewProj);

    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
	// Interpolating normal can unnormalize it, so renormalize it.
    PS_OUTPUT output;
    output.Color = float4(1, 0, 0, 1);
    return output;
}

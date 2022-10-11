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
#define MAX_BONE 100

cbuffer BoneDatas : register(b2)
{
    float4x4 cBoneTable[MAX_BONE];
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : UV;
	float3 TangentU : TANGENT;
    float3 BiTangentU : BITANGENT;
    uint4 Bone_Indices : BONE_IDS;
    float4 Weights : WEIGHTS;
    uint Weight_num : WEIGHT_NUM;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float4 PosW     : WORLD_POSITION;
    float3 NormalW  : NORMAL;
	float3 TangentW : UV;
	float2 TexC     : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 Position : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Albdeo : SV_Target2;
    float Roughness : SV_Target3;
    float Metalic : SV_Target4;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    float4x4 BoneTransform = cBoneTable[vin.Bone_Indices[0]] * vin.Weights[0];
    for(uint i = 1; i < vin.Weight_num; ++i)
    {
        BoneTransform += cBoneTable[vin.Bone_Indices[i]] * vin.Weights[i];
    }

    // Transform to homogeneous clip space.
    float4 posL = mul(float4(vin.PosL, 1.f), BoneTransform);
    //float4 posL = float4(vin.PosL, 1.f);
    float4 posW = mul(posL, objectWorld.mat);
    vout.PosW = posW;
    vout.PosH = mul(posW, gViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3)objectWorld.mat);
    vout.TangentW = mul(vin.TangentU, (float3x3)objectWorld.mat);

    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
	// Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);
	
    PS_OUTPUT output;
    output.Position = pin.PosW;
    output.Normal = float4(pin.NormalW, 1.0);
    output.Albdeo = float4(1.0, 1.0, 1.0, 1.0);
    output.Roughness = 0.1;
    output.Metalic = 1.0;
    return output;
}

struct Matrix
{
    matrix mat;
};

ConstantBuffer<Matrix> objectWorld : register(b0);
ConstantBuffer<Matrix> shadowVP : register(b1);

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
	float4 PosH  : SV_POSITION;
};

struct PS_OUTPUT
{
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), objectWorld.mat);
    vout.PosH = mul(posW, shadowVP.mat);

    return vout;
}

PS_OUTPUT PS(VertexOut pin)
{
    PS_OUTPUT output = (PS_OUTPUT)0;
    // perform pixel shading operations
    return output;
}

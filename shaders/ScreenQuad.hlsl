struct VertexOut
{
    float4 posH : SV_POSITION;
	float2 UV : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
	vout.UV = float2((vid << 1) & 2, vid & 2);
	vout.posH = float4(vout.UV * 2.0f + -1.0f, 0.0f, 1.0f);
	return vout;
}
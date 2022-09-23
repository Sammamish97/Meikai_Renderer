#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <vector>
#include <wrl.h>

#include "CommandList.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

using namespace Microsoft::WRL;
using namespace DirectX;

class DXApp;
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 UV;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
	
	UINT boneIDs[4];
	float weights[4];
	int weightNum = 0;

	void AddBoneData(int boneID, float weight);
};


struct Mesh
{
private:
	DXApp* mApp = nullptr;

public:
	Mesh(DXApp* dxApp, std::vector<Vertex> input_vertices, std::vector<WORD> input_indices, CommandList& commandList);
	void Draw(CommandList& commandList);

private:
	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	std::vector<Vertex> mVertices;
	std::vector<WORD> mIndices;

	UINT mIndexCount;
	UINT mBoneCount;

private:
	void Init(CommandList& commandList);
};


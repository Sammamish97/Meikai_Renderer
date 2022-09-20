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
struct BoneData
{
	std::string name;
	XMFLOAT4X4 offsetMatrix;
	XMINT4 jointIDs;
	XMFLOAT4 weights;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 UV;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
	XMFLOAT4 jointIDs;
	XMFLOAT4 weights;
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

private:
	void Init(CommandList& commandList);
};


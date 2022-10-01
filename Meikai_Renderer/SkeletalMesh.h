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
struct SkeletalVertex
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

class SkeletalMesh
{
private:
	DXApp* mApp = nullptr;

public:
	SkeletalMesh(DXApp* dxApp, std::vector<SkeletalVertex> input_vertices, std::vector<UINT> input_indices, CommandList& commandList);
	void Draw(CommandList& commandList);

private:
	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	std::vector<SkeletalVertex> mSkeletalVertices;
	std::vector<UINT> mIndices;

	UINT mIndexCount;
	UINT mBoneCount;

private:
	void Init(CommandList& commandList);
};


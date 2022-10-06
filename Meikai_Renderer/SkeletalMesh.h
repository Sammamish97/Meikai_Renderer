#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <map>
#include <vector>
#include <wrl.h>

#include "CommandList.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Animation;
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
	UINT weightNum = 0;

	void AddBoneData(int boneID, float weight);
};

/**
 * @brief Class for manage mesh with bone data.
 * @detail Skeletal mesh have boneID, weights and weight num for animating on shader.
 */
class SkeletalMesh
{
private:
	DXApp* mApp = nullptr;

public:
	SkeletalMesh(DXApp* dxApp, const aiScene* aiPtr, std::vector<SkeletalVertex> input_vertices, std::vector<UINT> input_indices, CommandList& commandList);

	void Draw(CommandList& commandList);
private:
	const aiScene* mScenePtr;

	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	std::vector<SkeletalVertex> mSkeletalVertices;
	std::vector<UINT> mIndices;

	UINT mIndexCount;

private:
	void Init(CommandList& commandList);
};


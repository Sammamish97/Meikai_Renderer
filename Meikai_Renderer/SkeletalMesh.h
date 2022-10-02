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

struct BoneData
{
	//Boen Data vector's Index == boneID
	aiMatrix4x4 offsetMatrix;
	aiMatrix4x4 finalMatrix;
	BoneData() = default;
	BoneData(const aiMatrix4x4& offset)
	{
		offsetMatrix = offset;
	}
};

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
	SkeletalMesh(DXApp* dxApp, const aiScene* aiPtr, std::vector<SkeletalVertex> input_vertices, std::vector<UINT> input_indices,
		std::vector<BoneData> boneData, std::map<std::string, UINT> boneMap, CommandList& commandList);
	void Draw(CommandList& commandList, float time, std::shared_ptr<Animation> animation);

	void DrawDebugJoints(CommandList& commandList);
	void DrawDebugBones(CommandList& commandList);

	void ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode, std::shared_ptr<Animation> animation, aiMatrix4x4& parentTransform);
	void GetBoneTransforms(float timeInSeconds, std::shared_ptr<Animation> animation, std::vector<aiMatrix4x4>& Transforms);


private:
	const aiScene* mScenePtr;

	VertexBuffer mVertexBuffer;
	IndexBuffer mIndexBuffer;

	std::vector<SkeletalVertex> mSkeletalVertices;
	std::vector<UINT> mIndices;

	std::vector<BoneData> mBoneData;
	std::map<std::string, UINT> mBoneMap;

	std::vector<XMFLOAT3> mJointPositions;
	std::vector<XMFLOAT3> mBonePositions;

	aiMatrix4x4 mGlobalInverseTransform;

	UINT mIndexCount;

private:
	void Init(CommandList& commandList);
};


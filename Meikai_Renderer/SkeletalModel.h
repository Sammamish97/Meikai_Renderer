#pragma once
#include <string>

#include <DirectXMath.h>
#include <wrl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <map>

#include "SkeletalMesh.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct DXApp;
class CommandList;
class Animation;

struct BoneData
{
	aiMatrix4x4 offsetMatrix;
	aiMatrix4x4 finalMatrix;
	BoneData() = default;
	BoneData(const aiMatrix4x4& offset)
	{
		offsetMatrix = offset;
	}
};

class SkeletalModel
{
private:
	DXApp* mApp;

public:
	SkeletalModel(const std::string& file_path, DXApp* app, CommandList& commandList);
	std::string name;

	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);
	SkeletalMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList);

	void DrawDebugJoints(CommandList& commandList);
	void DrawDebugBones(CommandList& commandList);

	void LoadVertices(aiMesh* mesh, std::vector<SkeletalVertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<UINT>& indices);
	void LoadBones(aiMesh* mesh, std::vector<SkeletalVertex>& vertices);

	void ExtractJoint();
	void ExtractBone();
	void ExtractBoneRecursive(const aiNode* pNode, aiVector3t<float> parentPos);

public:
	Assimp::Importer mImporter;
	const aiScene* pScene = nullptr;
	aiMatrix4x4 mGlobalInverseTransform;

	std::vector<SkeletalMesh> mMeshes;
	std::vector<BoneData> mBoneData;
	std::map<std::string, UINT> mBoneMap;

	std::vector<XMFLOAT3> mJointPositions;
	std::vector<XMFLOAT3> mBonePositions;
};


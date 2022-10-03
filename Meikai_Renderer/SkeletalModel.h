#pragma once
#include <string>

#include <DirectXMath.h>
#include <wrl.h>

#include <map>

#include "SkeletalMesh.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct DXApp;
class CommandList;
class Animation;

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

class SkeletalModel
{
private:
	DXApp* mApp;

public:
	SkeletalModel(const std::string& file_path, DXApp* app, CommandList& commandList);
	void Draw(CommandList& commandList, float time, std::shared_ptr<Animation> animation);

	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);
	SkeletalMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList);

	void LoadVertices(aiMesh* mesh, std::vector<SkeletalVertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<UINT>& indices);
	void LoadBones(aiMesh* mesh, std::vector<SkeletalVertex>& vertices);

	void ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode, std::shared_ptr<Animation> animation, aiMatrix4x4& parentTransform, aiVector3t<float> parentPos);
	void GetBoneTransforms(float timeInSeconds, std::shared_ptr<Animation> animation, std::vector<aiMatrix4x4>& Transforms);

	void DrawDebugJoints(CommandList& commandList);
	void DrawDebugBones(CommandList& commandList);

	void ExtractJoint();
	void ExtractBone();
	void ExtractBoneRecursive(const aiNode* pNode, aiVector3t<float> parentPos);

private:
	std::vector<BoneData> mBoneData;
	std::map<std::string, UINT> mBoneMap;

	aiMatrix4x4 mGlobalInverseTransform;
	std::vector<aiMatrix4x4> mFinalTransforms;

	std::vector<XMFLOAT3> mJointPositions;
	std::vector<XMFLOAT3> mBonePositions;

	UINT mTotalNumBones = 0;


public:
	std::string name;
	Assimp::Importer mImporter;
	const aiScene* pScene = nullptr;

	std::vector<SkeletalMesh> mMeshes;
};


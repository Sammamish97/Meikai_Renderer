#pragma once
#include <string>

#include <DirectXMath.h>
#include <wrl.h>

#include <map>

#include "AnimData.h"
#include "SkeletalMesh.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct DXApp;
class CommandList;
class Animation;

/**
 * @brief Class for manage model with bone data.
 * @detail Cache meshes which have bone data.
 */
class SkeletalModel
{
	friend Animation;
private:
	DXApp* mApp;

public:
	SkeletalModel(const std::string& file_path, DXApp* app, CommandList& commandList);
	void Draw(CommandList& commandList);

	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);
	SkeletalMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList);

	void LoadVertices(aiMesh* mesh, std::vector<SkeletalVertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<UINT>& indices);

	auto& GetBoneInfoMap() { return mBoneInfoMap; }
	UINT& GetBoneCount() { return mBoneCounter; }

	void ExtractBoneWeightForVertices(std::vector<SkeletalVertex>& vertices, aiMesh* mesh, const aiScene* scene);

private:
	//Bone information sorted by bond ID.
	std::map<std::string, BoneInfo> mBoneInfoMap;
	UINT mBoneCounter = 0;

public:
	std::string name;
	Assimp::Importer mImporter;
	const aiScene* pScene = nullptr;

	std::vector<SkeletalMesh> mMeshes;
};


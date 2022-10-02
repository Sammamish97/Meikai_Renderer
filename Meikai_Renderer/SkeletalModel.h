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

class SkeletalModel
{
private:
	DXApp* mApp;

public:
	SkeletalModel(const std::string& file_path, DXApp* app, CommandList& commandList);

	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);
	SkeletalMesh ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList);


	void LoadVertices(aiMesh* mesh, std::vector<SkeletalVertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<UINT>& indices);
	void LoadBones(aiMesh* mesh, std::vector<SkeletalVertex>& vertices
		, std::vector<BoneData>& boneVec, std::map<std::string, UINT>& boneMap);

public:
	std::string name;
	Assimp::Importer mImporter;
	const aiScene* pScene = nullptr;
	aiMatrix4x4 mGlobalInverseTransform;

	std::vector<SkeletalMesh> mMeshes;
};


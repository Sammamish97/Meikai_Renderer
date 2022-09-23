#pragma once
#include <string>

#include <DirectXMath.h>
#include <wrl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <map>

#include "Mesh.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct DXApp;
class CommandList;

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

struct Model
{
private:
	DXApp* mApp;

public:
	Model(const std::string& file_path, DXApp* app, CommandList& commandList);
	std::string name;

	void LoadModel(const std::string& file_path, CommandList& commandList);
	void ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList);

	void InitJointVertexBuffer(CommandList& commandList);
	void InitBoneVertexBuffer(CommandList& commandList);

	void UpdateGPUJointPosition(CommandList& commandList);
	void UpdateGPUBonePosition(CommandList& commandList);

	void DrawDebugJoints(CommandList& commandList);
	void DrawDebugBones(CommandList& commandList);

	void LoadVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<WORD>& indices);
	void LoadBones(aiMesh* mesh, std::vector<Vertex>& vertices);

	void ExtractJoint();
	void ExtractBone();
	void ExtractBoneRecursive(const aiNode* pNode, aiVector3t<float> parentPos);

	void GetBoneTransforms(std::vector<aiMatrix4x4>& Transforms);
	void ReadNodeHierarchy(const aiNode* pNode, const aiMatrix4x4& parentTransform);
public:

	Assimp::Importer mimporter;
	const aiScene* pScene = nullptr;

	std::vector<Mesh> mMeshes;
	std::vector<BoneData> mBoneData;
	std::map<std::string, UINT> mBoneMap;

	std::vector<XMFLOAT3> mJointPositions;
	std::vector<XMFLOAT3> mBonePositions;

	VertexBuffer mJointBuffer;
	VertexBuffer mBoneBuffer;
};


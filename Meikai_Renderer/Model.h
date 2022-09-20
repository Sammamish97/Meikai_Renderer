#pragma once
#include <string>

#include <DirectXMath.h>
#include <wrl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"

using namespace Microsoft::WRL;
using namespace DirectX;

struct DXApp;
class CommandList;

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

	void LoadVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
	void LoadIndices(aiMesh* mesh, std::vector<WORD>& indices);
	void LoadBones(aiMesh* mesh, std::vector<BoneData>& boneDatas, std::vector<Vertex>& vertices);

public:
	std::vector<Mesh> meshes;
};


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

struct Model
{
private:
	DXApp* mApp;
	ComPtr<ID3D12GraphicsCommandList2> mCommandList;
public:
	Model(const std::string& file_path, DXApp* app, ComPtr<ID3D12GraphicsCommandList2> commandList);
	std::string name;

	void LoadModel(const std::string& file_path);
	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	void SetCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);
	std::vector<Mesh> meshes;
};


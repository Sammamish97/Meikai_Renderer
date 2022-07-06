#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Model::Model(const std::string& file_path)
{
	LoadModel(file_path);
}

bool Model::LoadModel(const std::string& file_path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file_path,
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		assert("Fail to Load %s", file_path.c_str());
	}

	name = file_path.substr(0, file_path.find_last_of('/'));
	return true;
}

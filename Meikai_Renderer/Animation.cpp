#include "Animation.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void Animation::LoadAnimation(std::string file_path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file_path, 0);

}

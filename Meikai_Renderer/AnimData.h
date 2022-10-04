#pragma once
#include "assimp/matrix4x4.h"
struct BoneInfo
{
	/*id is index in finalBoneMatrices*/
	int id;

	/*offset matrix transforms vertex from model space to bone space*/
	aiMatrix4x4 offset;
};
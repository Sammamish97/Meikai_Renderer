#pragma once
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class DXApp;

//Each animation instance have only one aiAnimation
class Animation
{
private:
	DXApp* mApp;
	aiAnimation* mAnimation;

	Assimp::Importer mImporter;
	const aiScene* pScene;

public:
	Animation(DXApp* appPtr, const std::string& file_path);
	aiNodeAnim* FindNodeAnim(const std::string& nodeName);
	aiMatrix4x4 CalcNodeTransformation(aiNodeAnim* pNodeAnim, float AnimationTimeTicks);

private:
	void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim);
	unsigned FindPosition(float animationTimeTicks, const aiNodeAnim* pNodeAnim);

	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim);
	unsigned FindRotation(float animationTimeTicks, const aiNodeAnim* pNodeAnim);
	
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim);
	unsigned FindScaling(float animationTimeTicks, const aiNodeAnim* pNodeAnim);
};


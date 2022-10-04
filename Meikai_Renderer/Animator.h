#pragma once
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <memory>
#include "Animation.h"
#include "Bone.h"

class Animator
{
public:
	Animator(std::shared_ptr<Animation> animation);
	void UpdateAnimation(float dt, std::vector<aiVector3t<float>>& jointPosition, std::vector<aiVector3t<float>>& bonePosition);
	void PlayAnimation(std::shared_ptr<Animation> pAnimation);
	void CalculateBoneTransform(const AssimpNodeData* node, aiMatrix4x4 parentTransform, aiVector3t<float> parentPos, std::vector<aiVector3t<float>>& jointPosition, std::vector<aiVector3t<float>>& bonePosition);
	std::vector<aiMatrix4x4> GetFinalBoneMatrices();
	
private:
	std::vector<aiMatrix4x4> m_FinalBoneMatrices;
	std::shared_ptr<Animation> m_CurrentAnimation;
	float m_CurrentTime;
	float m_DeltaTime;
};


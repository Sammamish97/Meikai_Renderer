#pragma once
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <memory>
#include "Animation.h"
#include "Bone.h"
/**
 * @brief Class for manage animation playing.
 */
class Animator
{
public:
	Animator(std::shared_ptr<Animation> animation);
	/**
	* @brief Update m_FinalBoneMatrices by hierarchy structure.
	* @detail Need to call every frame for update bone constantly.
	* @param dt time get from update function
	* @param jointPosition positions for debug drawing
	* @param bonePosition positions for debug drawing.
	*/
	void UpdateAnimation(float dt, std::vector<aiVector3t<float>>& jointPosition, std::vector<aiVector3t<float>>& bonePosition);
	void PlayAnimation(std::shared_ptr<Animation> pAnimation);
	void CalculateBoneTransform(const AssimpNodeData* node, aiMatrix4x4 parentTransform, aiVector3t<float> parentPos, std::vector<aiVector3t<float>>& jointPosition, std::vector<aiVector3t<float>>& bonePosition);
	std::vector<aiMatrix4x4> GetFinalBoneMatrices();
	
private:
	aiMatrix4x4 m_GlobalInverse;
	std::vector<aiMatrix4x4> m_FinalBoneMatrices;
	std::shared_ptr<Animation> m_CurrentAnimation;
	float m_CurrentTick;
};


#pragma once
#include <map>
#include <vector>
#include <string>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Bone.h"
#include "AnimData.h"
#include "SkeletalModel.h"

class SkeletalModel;

/**
 * @brief Class for cache assimp hierarchy structure.
 */
struct AssimpNodeData
{
	aiMatrix4x4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

/**
 * @brief Class for represent animation.
 * @detail For construct animation, model require because bone set between animation and model can be different.
 */
class Animation
{
private:
	std::vector<Bone> mBones;
	std::map<std::string, BoneInfo> m_BoneInfoMap;

	float mTickPerSec;
	float mDuration;
	AssimpNodeData mRootNode;

public:

	inline float GetTicksPerSecond() { return mTickPerSec; }
	inline float GetDuration() { return mDuration; }
	inline const AssimpNodeData& GetRootNode() { return mRootNode; }
	inline const std::map<std::string, BoneInfo>& GetBoneIDMap()
	{
		return m_BoneInfoMap;
	}

	Animation(const std::string& animationPath, std::shared_ptr<SkeletalModel> model);
	~Animation();
	Bone* FindBone(const std::string& name);

private:
	/**
	* @brief Function for match bone set between animation and model.
	* @detail If there are bone which exist on model but don't exist on animation, put that bone in the animation's bone set.
	*/
	void ReadMissingBones(const aiAnimation* animation, std::shared_ptr<SkeletalModel> model);

	/**
	* @brief Cache whole assimp structure recursively.
	*/
	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);
};


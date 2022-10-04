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

struct AssimpNodeData
{
	aiMatrix4x4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

//Each animation instance have only one aiAnimation
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
	void ReadMissingBones(const aiAnimation* animation, std::shared_ptr<SkeletalModel> model);
	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);
};


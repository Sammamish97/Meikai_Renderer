#include "Animation.h"
#include "DXApp.h"

#include <cassert>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "SkeletalModel.h"
Animation::Animation(const std::string& animationPath, std::shared_ptr<SkeletalModel> model)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(animationPath, aiProcess_ConvertToLeftHanded | aiProcess_Triangulate);
	assert(scene && scene->mRootNode);
	auto animation = scene->mAnimations[0];
	mDuration = animation->mDuration;
	mTickPerSec = animation->mTicksPerSecond;
	aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
	globalTransformation = globalTransformation.Inverse();
	ReadHeirarchyData(mRootNode, scene->mRootNode);
	ReadMissingBones(animation, model);
}

Animation::~Animation()
{
}

Bone* Animation::FindBone(const std::string& name)
{
	auto iter = std::find_if(mBones.begin(), mBones.end(),
		[&](const Bone& Bone)
		{
			return Bone.GetBoneName() == name;
		}
	);
	if (iter == mBones.end()) return nullptr;
	else return &(*iter);
}

void Animation::ReadMissingBones(const aiAnimation* animation, std::shared_ptr<SkeletalModel> model)
{
	int size = animation->mNumChannels;

	auto& boneInfoMap = model->GetBoneInfoMap();//getting m_BoneInfoMap from Model class
	UINT& boneCount = model->GetBoneCount(); //getting the m_BoneCounter from Model class

	//reading channels(bones engaged in an animation and their keyframes)
	for (int i = 0; i < size; i++)
	{
		auto channel = animation->mChannels[i];
		std::string boneName = channel->mNodeName.data;

		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			boneInfoMap[boneName].id = boneCount;
			boneCount++;
		}
		mBones.push_back(Bone(channel->mNodeName.data,
			boneInfoMap[channel->mNodeName.data].id, channel));
	}

	m_BoneInfoMap = boneInfoMap;
}

void Animation::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
	assert(src);

	dest.name = src->mName.data;
	dest.transformation = src->mTransformation;
	dest.childrenCount = src->mNumChildren;

	for (int i = 0; i < src->mNumChildren; i++)
	{
		AssimpNodeData newData;
		ReadHeirarchyData(newData, src->mChildren[i]);
		dest.children.push_back(newData);
	}
}
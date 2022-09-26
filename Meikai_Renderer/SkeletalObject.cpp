#include "SkeletalObject.h"
#include "Animation.h"

#include "Model.h"

void SkeletalObject::GetBoneTransforms(float timeInSeconds, std::vector<aiMatrix4x4>& Transforms)
{
    ReadNodeHierarchy(timeInSeconds, mModel->pScene->mRootNode, aiMatrix4x4());

    UINT boneDataSize = mModel->mBoneData.size();
    Transforms.resize(mModel->mBoneData.size());
    for (UINT i = 0; i < boneDataSize; ++i)
    {
        Transforms[i] = mModel->mBoneData[i].finalMatrix;
    }
}

void SkeletalObject::ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode, const aiMatrix4x4& parentTransform)
{
    std::string nodeName(pNode->mName.data);
    aiMatrix4x4 nodeTransformation(pNode->mTransformation);
    aiMatrix4x4 globalTransformation = parentTransform * nodeTransformation;

    aiNodeAnim* pNodeAnim = mAnimation->FindNodeAnim(nodeName);
    if (pNodeAnim)
    {
        nodeTransformation = mAnimation->CalcNodeTransformation(pNodeAnim, timeInSeconds);
    }

    if (mModel->mBoneMap.find(nodeName) != mModel->mBoneMap.end())
    {
        UINT boneIndex = mModel->mBoneMap[nodeName];
        mModel->mBoneData[boneIndex].finalMatrix = globalTransformation * mModel->mBoneData[boneIndex].offsetMatrix;
    }

    for (UINT i = 0; i < pNode->mNumChildren; ++i)
    {
        ReadNodeHierarchy(timeInSeconds, pNode->mChildren[i], globalTransformation);
    }
}

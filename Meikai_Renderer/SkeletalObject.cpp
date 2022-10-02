#include "SkeletalObject.h"
#include "SkeletalModel.h"
#include "Animation.h"

#include "SkeletalModel.h"

SkeletalObject::SkeletalObject(std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim, XMFLOAT3 position,
	XMFLOAT3 scale)
    :mModel(model), mPosition(position), mScale(scale), mAnimation(initAnim), mPlayTime(0)
{
}

void SkeletalObject::Update(float dt)
{

}

void SkeletalObject::Draw(CommandList& commandList)
{
    std::vector<aiMatrix4x4> finalTransforms;
    GetBoneTransforms(1.f, finalTransforms);
    commandList.SetGraphicsDynamicConstantBuffer(2, finalTransforms.size() * sizeof(aiMatrix4x4), finalTransforms.data());
    SetWorldMatrix(commandList);
    for (auto& mesh : mModel->mMeshes)
    {
        mesh.Draw(commandList);
    }
}

XMMATRIX SkeletalObject::GetWorldMat() const
{
    XMMATRIX translationMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
    XMMATRIX scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

    return XMMatrixMultiply(translationMat, scaleMat);
}

void SkeletalObject::SetWorldMatrix(CommandList& commandList)
{
    XMMATRIX worldMat = GetWorldMat();
    worldMat = XMMatrixTranspose(worldMat);
    commandList.SetGraphics32BitConstants(0, worldMat);
}

void SkeletalObject::PlayAnimation()
{
}

void SkeletalObject::DrawJoint(CommandList& commandList)
{
    SetWorldMatrix(commandList);
    mModel->DrawDebugJoints(commandList);
}

void SkeletalObject::DrawBone(CommandList& commandList)
{
    SetWorldMatrix(commandList);
    mModel->DrawDebugBones(commandList);
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
        mModel->mBoneData[boneIndex].finalMatrix = mModel->mGlobalInverseTransform * globalTransformation * mModel->mBoneData[boneIndex].offsetMatrix.Inverse();
    }

    for (UINT i = 0; i < pNode->mNumChildren; ++i)
    {
        ReadNodeHierarchy(timeInSeconds, pNode->mChildren[i], globalTransformation);
    }
}

void SkeletalObject::GetBoneTransforms(float timeInSeconds, std::vector<aiMatrix4x4>& Transforms)
{
    ReadNodeHierarchy(timeInSeconds, mModel->pScene->mRootNode, aiMatrix4x4());

    UINT boneDataSize = mModel->mBoneData.size();
    Transforms.resize(mModel->mBoneData.size());
    for (UINT i = 0; i < boneDataSize; ++i)
    {
        Transforms[i] = mModel->mBoneData[i].finalMatrix.Transpose();//Transpose because DX12 use row major. Therefore, transform.
    }
}

void SkeletalObject::SetAnimation(std::shared_ptr<Animation> newAnimation)
{
    mAnimation = newAnimation;
}

void SkeletalObject::SetDynamicBoneMatrices(CommandList& commandList)
{
    //Update matrices on Constant buffer.
}

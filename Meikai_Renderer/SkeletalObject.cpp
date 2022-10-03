#include "SkeletalObject.h"
#include "SkeletalModel.h"
#include "Animation.h"
#include <cmath>

#include "SkeletalModel.h"

SkeletalObject::SkeletalObject(std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim, XMFLOAT3 position,
	XMFLOAT3 scale)
    :mModel(model), mPosition(position), mScale(scale), mCurrentAnimation(initAnim), mPlayTime(0)
{
}

void SkeletalObject::Update(float dt)
{

}

void SkeletalObject::Draw(CommandList& commandList)
{
    mPlayTime += 0.016;
    SetWorldMatrix(commandList);
    mModel->Draw(commandList, std::fmod(mPlayTime,mCurrentAnimation->mDuration), mCurrentAnimation);
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

void SkeletalObject::SetAnimation(std::shared_ptr<Animation> newAnimation)
{
    mCurrentAnimation = newAnimation;
}

void SkeletalObject::SetDynamicBoneMatrices(CommandList& commandList)
{
    //Update matrices on Constant buffer.
}

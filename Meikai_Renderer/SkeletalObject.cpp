#include "SkeletalObject.h"
#include "SkeletalModel.h"
#include "Animator.h"
#include "DXApp.h"
#include <cmath>

#include "Model.h"

SkeletalObject::SkeletalObject(DXApp* appPtr, std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim, XMFLOAT3 position,
	XMFLOAT3 scale)
    :mApp(appPtr), mModel(model), mAnimation(initAnim), mAnimator(mAnimation), mPosition(position), mScale(scale)
{
    mAnimator.PlayAnimation(mAnimation);

}

void SkeletalObject::Update(float dt)
{
    mJointPositions.clear();
    mBonePositions.clear();
    mAnimator.UpdateAnimation(dt, mJointPositions, mBonePositions);
}

void SkeletalObject::SetAnimator(std::shared_ptr<Animation> newAnimation)
{
    mAnimation = newAnimation;
    mAnimator.PlayAnimation(mAnimation);
;}

void SkeletalObject::Draw(CommandList& commandList)
{
    auto finalMatrices = mAnimator.GetFinalBoneMatrices();
    commandList.SetGraphicsDynamicConstantBuffer(2, finalMatrices.size() * sizeof(aiMatrix4x4), finalMatrices.data());
	SetWorldMatrix(commandList);
    mModel->Draw(commandList);
}

void SkeletalObject::DrawJoint(CommandList& commandList)
{
   
    auto vertexCount = mJointPositions.size();
    auto vertexSize = sizeof(mJointPositions[0]);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    SetWorldMatrix(commandList);
    commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mJointPositions.data());
    commandList.Draw(vertexCount);
}

void SkeletalObject::DrawBone(CommandList& commandList)
{
    SetWorldMatrix(commandList);
    auto vertexCount = mBonePositions.size();
    auto vertexSize = sizeof(mBonePositions[0]);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mBonePositions.data());
    commandList.Draw(vertexCount);
}

XMMATRIX SkeletalObject::GetWorldMat() const
{
    XMMATRIX scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
    XMMATRIX translationMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);

    XMMATRIX result;
    if(mDirection.x == 0.f && mDirection.y == 0.f && mDirection.z == 0.f)
    {
	    result = translationMat * scaleMat;
    }
    else
    {
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR dir = XMLoadFloat3(&mDirection);
        XMVECTOR position = XMLoadFloat3(&mPosition);
        XMVECTOR target = position + dir;
        XMMATRIX lookAt = XMMatrixLookAtLH(target, position, up);
        XMFLOAT4X4 rotationPart;
        XMStoreFloat4x4(&rotationPart, lookAt);

        lookAt = XMLoadFloat4x4(&rotationPart);
        lookAt = XMMatrixInverse(nullptr, lookAt);
        result = lookAt * scaleMat;
    }

    return result;
}

void SkeletalObject::SetWorldMatrix(CommandList& commandList)
{
    XMMATRIX worldMat = GetWorldMat();
    worldMat = XMMatrixTranspose(worldMat);
    commandList.SetGraphics32BitConstants(0, worldMat);
}

void SkeletalObject::SetPosition(XMVECTOR newPos)
{
    XMStoreFloat3(&mPosition, newPos);
}

void SkeletalObject::SetDirection(XMVECTOR newDir)
{
    XMStoreFloat3(&mDirection, newDir);

}


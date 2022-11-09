#include "IkObject.h"

#include "MaterialData.h"

IkObject::IkObject(DXApp* appPtr, std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim,
                   XMFLOAT3 position, XMFLOAT3 albedo, float metalic, float roughness, XMFLOAT3 scale)
	:mApp(appPtr), mModel(model), mAnimation(initAnim), mAnimator(mAnimation), mPosition(position),
	mAlbedo(albedo), mMetalic(metalic), mRoughness(roughness), mScale(scale)
{
	mAnimator.PlayAnimation(mAnimation);
}

void IkObject::Update(float tick)
{
	mJointPositions.clear();
	mBonePositions.clear();
	mAnimator.UpdateAnimation(tick, mJointPositions, mBonePositions);
	//TODO: 여기서 mAnimator대신 IK Resolver가 들어가야 할 것이다.
}

void IkObject::Draw(CommandList& commandList)
{
	auto finalMatrices = mAnimator.GetFinalBoneMatrices();
	commandList.SetGraphicsDynamicConstantBuffer(2, finalMatrices.size() * sizeof(aiMatrix4x4), finalMatrices.data());
	SetWorldMatrix(commandList);
	SetMaterial(commandList);
	mModel->Draw(commandList);
}

void IkObject::DrawJoint(CommandList& commandList)
{
	auto vertexCount = mJointPositions.size();
	auto vertexSize = sizeof(mJointPositions[0]);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	SetWorldMatrix(commandList);
	commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mJointPositions.data());
	commandList.Draw(vertexCount);
}

void IkObject::DrawBone(CommandList& commandList)
{
	SetWorldMatrix(commandList);
	auto vertexCount = mBonePositions.size();
	auto vertexSize = sizeof(mBonePositions[0]);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mBonePositions.data());
	commandList.Draw(vertexCount);
}

XMMATRIX IkObject::GetWorldMat() const
{
	XMMATRIX scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);
	XMMATRIX translationMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);

	XMMATRIX result;
	if (mDirection.x == 0.f && mDirection.y == 0.f && mDirection.z == 0.f)
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

		for (int i = 0; i < 3; ++i)
		{
			rotationPart.m[3][i] = 0.f;
		}
		lookAt = XMLoadFloat4x4(&rotationPart);

		lookAt = XMMatrixInverse(nullptr, lookAt);
		result = scaleMat * lookAt * translationMat;
	}

	return result;
}

float IkObject::GetTicksPerSec()
{
	return mAnimation->GetTicksPerSecond();
}

float IkObject::GetDuration()
{
	return mAnimation->GetDuration();
}

float IkObject::GetDistacnePerDuration()
{
	return mAnimation->GetDistancePerDuration();
}

void IkObject::SetWorldMatrix(CommandList& commandList)
{
	XMMATRIX worldMat = GetWorldMat();
	worldMat = XMMatrixTranspose(worldMat);
	commandList.SetGraphics32BitConstants(0, worldMat);
}

void IkObject::SetMaterial(CommandList& commandList)
{
	MaterialData matData(mAlbedo, mMetalic, mRoughness);
	commandList.SetGraphics32BitConstants(3, matData);
}

void IkObject::SetPosition(XMVECTOR newPos)
{
	XMStoreFloat3(&mPosition, newPos);
}

void IkObject::SetDirection(XMVECTOR newDir)
{
	XMStoreFloat3(&mDirection, newDir);
}

void IkObject::SetAlbedo(XMFLOAT3 newAlbedo)
{
	mAlbedo = newAlbedo;
}

void IkObject::SetMetalic(float newMetalic)
{
	mMetalic = newMetalic;
}

void IkObject::SetRoughness(float newRoughness)
{
	mRoughness = newRoughness;
}

void IkObject::SetAnimator(std::shared_ptr<Animation> newAnimation)
{
	mAnimation = newAnimation;
	mAnimator.PlayAnimation(mAnimation);
}

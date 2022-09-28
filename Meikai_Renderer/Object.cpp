#include "Object.h"
#include "Model.h"
#include "CommandList.h"


Object::Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 scale)
	:mModel(model), mPosition(position), mScale(scale)
{
}

void Object::SetWorldMatrix(CommandList& commandList)
{
	XMMATRIX worldMat = GetWorldMat();
	worldMat = XMMatrixTranspose(worldMat);
	commandList.SetGraphics32BitConstants(0, worldMat);
}

void Object::Update(float dt)
{
}

XMMATRIX Object::GetWorldMat() const
{
	XMMATRIX translationMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	XMMATRIX scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

	return XMMatrixMultiply(translationMat, scaleMat);
}

void Object::Draw(CommandList& commandList)
{
	for (auto& mesh : mModel->mMeshes)
	{
		SetWorldMatrix(commandList);
		mesh.Draw(commandList);
	}
}

void Object::DrawWithoutWorld(CommandList& commandList)
{
	for (auto& mesh : mModel->mMeshes)
	{
		mesh.Draw(commandList);
	}
}

void Object::DrawJoint(CommandList& commandList)
{
	SetWorldMatrix(commandList);
	mModel->DrawDebugJoints(commandList);
}

void Object::DrawBone(CommandList& commandList)
{
	SetWorldMatrix(commandList);
	mModel->DrawDebugBones(commandList);
}


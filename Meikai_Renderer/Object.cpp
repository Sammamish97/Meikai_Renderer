#include "Object.h"
#include "Model.h"
#include "CommandList.h"
#include "MaterialData.h"


Object::Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 albedo, float metalic, float roughness,  XMFLOAT3 scale)
	:mModel(model), mPosition(position), mAlbedo(albedo), mMetalic(metalic), mRoughness(roughness), mScale(scale)
{
}

void Object::SetWorldMatrix(CommandList& commandList)
{
	XMMATRIX worldMat = GetWorldMat();
	worldMat = XMMatrixTranspose(worldMat);
	commandList.SetGraphics32BitConstants(0, worldMat);
}

void Object::SetMaterial(CommandList& commandList)
{
	MaterialData matData(mAlbedo, mMetalic, mRoughness);
	commandList.SetGraphics32BitConstants(2, matData);
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
	SetWorldMatrix(commandList);
	SetMaterial(commandList);
	for (auto& mesh : mModel->mMeshes)
	{
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

void Object::SetPosition(XMVECTOR newPos)
{
	XMStoreFloat3(&mPosition, newPos);
}

void Object::SetAlbedo(XMFLOAT3 newAlbedo)
{
	mAlbedo = newAlbedo;
}

void Object::SetMetalic(float newMetalic)
{
	mMetalic = newMetalic;
}

void Object::SetRoughness(float newRoughness)
{
	mRoughness = newRoughness;
}

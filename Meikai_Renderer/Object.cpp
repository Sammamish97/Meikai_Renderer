#include "Object.h"
#include "Model.h"

Object::Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 scale)
	:mModel(model), mPosition(position), mScale(scale)
{
}

void Object::SetWorldMatrix(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	XMMATRIX worldMat = GetWorldMat();
	worldMat = XMMatrixTranspose(worldMat);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &worldMat, 0);
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

void Object::Draw(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	for (const auto& mesh : mModel->meshes)
	{
		SetWorldMatrix(commandList);
		commandList->IASetVertexBuffers(0, 1, &mesh.m_VertexBufferView);
		commandList->IASetIndexBuffer(&mesh.m_IndexBufferView);

		commandList->DrawIndexedInstanced(mesh.m_indices.size(), 1, 0, 0, 0);
	}
}

void Object::DrawWithoutWorld(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	for (const auto& mesh : mModel->meshes)
	{
		commandList->IASetVertexBuffers(0, 1, &mesh.m_VertexBufferView);
		commandList->IASetIndexBuffer(&mesh.m_IndexBufferView);

		commandList->DrawIndexedInstanced(mesh.m_indices.size(), 1, 0, 0, 0);
	}
}
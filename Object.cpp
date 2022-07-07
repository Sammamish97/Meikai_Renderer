#include "Object.h"

#include "Model.h"

Object::Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 scale)
	:mModel(model), mPosition(position), mScale(scale)
{
}

void Object::SetMVPMatrix(ComPtr<ID3D12GraphicsCommandList2> commandList, XMMATRIX viewMat, XMMATRIX projMat)
{
	XMMATRIX translationMat = XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
	XMMATRIX scaleMat = XMMatrixScaling(mScale.x, mScale.y, mScale.z);

	XMMATRIX worldMat = XMMatrixMultiply(translationMat, scaleMat);

	XMMATRIX mvpMatrix = XMMatrixMultiply(worldMat, viewMat);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, projMat);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
}


void Object::Update(float dt)
{
}

void Object::Draw(ComPtr<ID3D12GraphicsCommandList2> commandList, XMMATRIX viewMat, XMMATRIX projMat)
{
	for (const auto& mesh : mModel->meshes)
	{
		SetMVPMatrix(commandList, viewMat, projMat);
		commandList->IASetVertexBuffers(0, 1, &mesh.m_VertexBufferView);
		commandList->IASetIndexBuffer(&mesh.m_IndexBufferView);

		commandList->DrawIndexedInstanced(mesh.m_indices.size(), 1, 0, 0, 0);
	}
}

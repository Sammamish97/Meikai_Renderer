#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>
#include <memory>

using namespace Microsoft::WRL;
using namespace DirectX;

struct Model;
struct Object
{
	Object(std::shared_ptr<Model>, XMFLOAT3 position, XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));

	void Update(float dt);
	void Draw(ComPtr<ID3D12GraphicsCommandList2> commandList);

	XMMATRIX GetWorldMat() const;
private:
	void SetWorldMatrix(ComPtr<ID3D12GraphicsCommandList2> commandList);

	std::shared_ptr<Model> mModel = nullptr;
	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
};

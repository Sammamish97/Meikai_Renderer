#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;

struct Model;
struct Object
{
public:
	Object(Model* model, XMFLOAT3 position, XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));

	
	void Update(float dt);
	void Draw(ComPtr<ID3D12GraphicsCommandList2> commandList, XMMATRIX viewMat, XMMATRIX projMat);

private:
	void SetMVPMatrix(ComPtr<ID3D12GraphicsCommandList2> commandList, XMMATRIX viewMat, XMMATRIX projMat);

	Model* mModel = nullptr;
	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
};


#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <wrl.h>
#include <memory>

using namespace Microsoft::WRL;
using namespace DirectX;
class CommandList;
struct Model;
struct Object
{
	Object(std::shared_ptr<Model> model, XMFLOAT3 position, XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));

	void Update(float dt);
	void Draw(CommandList& commandList);
	void DrawJoint(CommandList& commandList);
	void DrawBone(CommandList& commandList);
	void DrawWithoutWorld(CommandList& commandList);
	XMMATRIX GetWorldMat() const;
private:
	void SetWorldMatrix(CommandList& commandList);

	std::shared_ptr<Model> mModel = nullptr;
	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
};


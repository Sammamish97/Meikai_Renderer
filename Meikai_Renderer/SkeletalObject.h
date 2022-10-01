#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <wrl.h>
#include <memory>
#include "Object.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Microsoft::WRL;
using namespace DirectX;
class CommandList;
class Animation;
class SkeletalModel;

class SkeletalObject
{
public:
	SkeletalObject(std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim, XMFLOAT3 position,  XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));
	void Update(float dt);
	void Draw(CommandList& commandList);

	void DrawJoint(CommandList& commandList);
	void DrawBone(CommandList& commandList);

	XMMATRIX GetWorldMat() const;
	void SetWorldMatrix(CommandList& commandList);

	void SetAnimation(std::shared_ptr<Animation> newAnimation);
	void GetBoneTransforms(float timeInSeconds, std::vector<aiMatrix4x4>& Transforms);
	void ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode, const aiMatrix4x4& parentTransform);

	void PlayAnimation();

	void SetDynamicBoneMatrices(CommandList& commandList);

private:
	std::shared_ptr<Animation> mAnimation = nullptr;
	double mPlayTime;

	std::shared_ptr<SkeletalModel> mModel = nullptr;
	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
};


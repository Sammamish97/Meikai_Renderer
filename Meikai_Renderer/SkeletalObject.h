#pragma once
#include <DirectXMath.h>
#include <d3dx12.h>
#include <wrl.h>
#include <memory>
#include "Object.h"
#include "Animator.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Microsoft::WRL;
using namespace DirectX;
class CommandList;
class Animation;
class SkeletalModel;
class DXApp;

/**
 * @brief Class for manage models with animation.
 * @detail Capsulize three objects. Animation, animator and model.
 */
class SkeletalObject
{
private:
	DXApp* mApp;
public:
	SkeletalObject(DXApp* appPtr, std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim, XMFLOAT3 position,  XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));
	void Update(float dt);
	void Draw(CommandList& commandList);

	void DrawJoint(CommandList& commandList);
	void DrawBone(CommandList& commandList);

	XMMATRIX GetWorldMat() const;
	void SetWorldMatrix(CommandList& commandList);

	void SetAnimator(std::shared_ptr<Animation> newAnimation);

private:
	std::shared_ptr<SkeletalModel> mModel = nullptr;
	std::shared_ptr<Animation> mAnimation = nullptr;

	std::vector<aiVector3t<float>> mJointPositions;
	std::vector<aiVector3t<float>> mBonePositions;

	Animator mAnimator;
	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
};


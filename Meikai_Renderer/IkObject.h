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
class IkObject
{
private:
	DXApp* mApp;
public:
	IkObject(DXApp* appPtr, std::shared_ptr<SkeletalModel> model, std::shared_ptr<Animation> initAnim, XMFLOAT3 position,
		XMFLOAT3 mAlbedo, float metalic, float roughness, XMFLOAT3 scale = XMFLOAT3(1.f, 1.f, 1.f));
	void Update(float tick);
	void Draw(CommandList& commandList);

	void DrawJoint(CommandList& commandList);
	void DrawBone(CommandList& commandList);

	XMMATRIX GetWorldMat() const;
	float GetTicksPerSec();
	float GetDuration();
	float GetDistacnePerDuration();
	void SetWorldMatrix(CommandList& commandList);
	void SetMaterial(CommandList& commandList);

	void SetPosition(XMVECTOR newPos);
	void SetDirection(XMVECTOR newDir);
	void SetAlbedo(XMFLOAT3 newAlbedo);
	void SetMetalic(float newMetalic);
	void SetRoughness(float newRoughness);


	void SetAnimator(std::shared_ptr<Animation> newAnimation);
private:
	XMFLOAT3 mAlbedo;
	float mMetalic;
	float mRoughness;

	std::shared_ptr<SkeletalModel> mModel = nullptr;
	std::shared_ptr<Animation> mAnimation = nullptr;

	std::vector<aiVector3t<float>> mJointPositions;
	std::vector<aiVector3t<float>> mBonePositions;

	Animator mAnimator;

	XMFLOAT3 mPosition;
	XMFLOAT3 mScale;
	XMFLOAT3 mDirection;
};


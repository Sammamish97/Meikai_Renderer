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
class Model;

class SkeletalObject : public Object
{
public:
	void Update(float dt) override;

	void SetAnimation(std::shared_ptr<Animation> newAnimation);
	void GetBoneTransforms(float timeInSeconds, std::vector<aiMatrix4x4>& Transforms);
	void ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode, const aiMatrix4x4& parentTransform);

	void SetDynamicBoneMatrices(CommandList& commandList);

private:
	std::shared_ptr<Animation> mAnimation = nullptr;
	//Neet to manager animation time something.
};


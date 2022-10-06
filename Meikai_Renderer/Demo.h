#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "FrameBufferResource.h"
#include "DXApp.h"
#include "ConstantBuffers.h"

class Model;
class SkeletalModel;

class Object;
class SkeletalObject;

class Animation;
class Animator;

class Camera;
class Texture;

class SkeletalGeometryPass;
class EquiRectToCubemapPass;
class CalcIBLDiffusePass;
class DefaultPass;
class GeometryPass;
class LightingPass;
class JointDebugPass;
class BoneDebugPass;
class SkyboxPass;

class Demo : public DXApp
{
public:
	Demo(HINSTANCE hInstance);
	~Demo();

	bool Initialize() override;

protected:
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;

private:
	void BuildModels(std::shared_ptr<CommandList>& cmdList);
	void LoadAnimations();
	void BuildObjects();

	void BuildFrameResource();
	void CreateIBLResources(std::shared_ptr<CommandList>& commandList);
	void CreateShader();
	void PreCompute();

	void DrawDefaultPass(CommandList& commandList);
	void DrawGeometryPasses(CommandList& cmdList);
	void DrawLightingPass(CommandList& cmdList);
	void DrawJointDebug(CommandList& cmdList);
	void DrawBoneDebug(CommandList& cmdList);
	void DrawSkyboxPass(CommandList& cmdList);

	void DispatchEquiRectToCubemap(CommandList& cmdList);
	void DispatchIBLDiffuse(CommandList& cmdList);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

private:
	void PreCalculateIBL();
	void CalcDiffuseIrradiance();

private:
	void UpdatePassCB(const GameTimer& gt);
	void UpdateLightCB(const GameTimer& gt);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private://Non-Iterating Pass
	std::unique_ptr<EquiRectToCubemapPass> mEquiRectToCubemapPass;
	std::unique_ptr<CalcIBLDiffusePass> mCalcIBLDiffusePass;

private://Passes
	std::unique_ptr<DefaultPass> mDefaultPass;

	std::unique_ptr<GeometryPass> mGeometryPass;
	std::unique_ptr<SkeletalGeometryPass> mSkeletalGeometryPass;
	std::unique_ptr<LightingPass> mLightingPass;
	std::unique_ptr<SkyboxPass> mSkyboxPass;

private://Debug Passes
	std::unique_ptr<JointDebugPass> mJointDebugPass;
	std::unique_ptr<BoneDebugPass> mBoneDebugPass;


private://CBV resource & allocation
	std::unique_ptr<CommonCB> mCommonCB;
	std::unique_ptr<LightCB> mLightCB;

private:
	IBLResource mIBLResource;
	IBLDescriptorIndex mIBLIndex;

private:
	std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
	std::unordered_map<std::string, std::shared_ptr<SkeletalModel>> mSkeletalModels;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::shared_ptr<Animation>> mAnimations;
	std::shared_ptr<Animator> testAnimator;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<Object>> mObjects;
	std::vector<std::unique_ptr<SkeletalObject>> mSkeletalObjects;

	std::unique_ptr<Object> mSkybox;

	std::unique_ptr<Camera> mCamera;
	POINT mLastMousePos;

	bool m_ContentLoaded = false;

	int blurCount = 5;
};


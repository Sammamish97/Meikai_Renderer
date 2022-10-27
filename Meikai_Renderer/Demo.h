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

class PathGenerator;

class SkeletalGeometryPass;
class EquiRectToCubemapPass;
class CalcIBLDiffusePass;
class GeometryPass;
class LightingPass;
class DebugMeshPass;
class DebugLinePass;
class SkyboxPass;
class ShadowPass;
class SsaoPass;
class BlurPass;

class Demo : public DXApp
{
public:
	Demo(HINSTANCE hInstance);
	~Demo();

	bool Initialize() override;
	void InitImgui();
protected:
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;

	void StartImGuiFrame();
	void ClearImGui();

private:
	void BuildModels(std::shared_ptr<CommandList>& cmdList);
	void LoadAnimations();
	void BuildObjects();

	void BuildFrameResource();
	void CreateIBLResources(std::shared_ptr<CommandList>& commandList);
	void CreateShaderFromHLSL();
	void CreateShaderFromCSO();
	void PreCompute();

	void DrawGeometryPasses(CommandList& cmdList);
	void DrawLightingPass(CommandList& cmdList);
	void DrawJointDebug(CommandList& cmdList);
	void DrawBoneDebug(CommandList& cmdList);
	void DrawPathDebug(CommandList& cmdList);
	void DrawMeshDebug(CommandList& cmdList);
	void DrawSkyboxPass(CommandList& cmdList);
	void DrawGUI(CommandList& commandList);
	void DrawShadowPass(CommandList& commandList);
	void DrawSsaoPass(CommandList& commandList);

	void DispatchEquiRectToCubemap(CommandList& cmdList);
	void DispatchBluring(CommandList& cmdList);

private:
	void UpdatePassCB(const GameTimer& gt);
	void UpdateLightCB(const GameTimer& gt);
	XMFLOAT4X4 BuildShadowMatrix(bool isShadowPass);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private://Non-Iterating Pass
	std::unique_ptr<EquiRectToCubemapPass> mEquiRectToCubemapPass;

private://Passes

	std::unique_ptr<GeometryPass> mGeometryPass;
	std::unique_ptr<SkeletalGeometryPass> mSkeletalGeometryPass;
	std::unique_ptr<LightingPass> mLightingPass;
	std::unique_ptr<SkyboxPass> mSkyboxPass;
	std::unique_ptr<ShadowPass> mShadowPass;
	std::unique_ptr<SsaoPass> mSsaoPass;

	std::unique_ptr<BlurPass> mBlurHPass;
	std::unique_ptr<BlurPass> mBlurVPass;


private://Debug Passes
	std::unique_ptr<DebugMeshPass> mJointDebugPass;
	std::unique_ptr<DebugLinePass> mBoneDebugPass;


private://CBV resource & allocation
	std::unique_ptr<CommonCB> mCommonCB;
	std::unique_ptr<LightCB> mLightCB;
	std::unique_ptr<RandomSampleCB> mRandomSampleCB;

private:
	IBLResource mIBLResource;
	IBLDescriptorIndex mIBLIndex;

private:
	ComPtr<ID3D12DescriptorHeap> g_pd3dRtvDescHeap = NULL;
	ComPtr<ID3D12DescriptorHeap> g_pd3dSrvDescHeap = NULL;

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
	std::unique_ptr<SkeletalObject> mMoveTestSkeletal;

	std::unique_ptr<Camera> mCamera;

	std::unique_ptr<PathGenerator> mPathGenerator;
	POINT mLastMousePos;

	bool m_ContentLoaded = false;

	int blurCount = 5;
};


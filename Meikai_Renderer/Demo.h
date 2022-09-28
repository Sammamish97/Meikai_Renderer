#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "FrameBufferResource.h"
#include "DXApp.h"
#include "ConstantBuffers.h"


class Model;
class Object;
class Camera;

class EquiRectToCubemapPass;
class DefaultPass;
class GeometryPass;
class LightingPass;
class JointDebugPass;
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
	void BuildFrameResource();
	void CreateIBLResources(std::shared_ptr<CommandList>& commandList);
	void CreateShader();
	void EquiRectToCubemap();
private:
	void CreateDescriptorHeaps();

	void CreateBufferResources();

	void CreateBufferDescriptors();
	void CreateIBLDescriptors();

	void DrawDefaultPass(CommandList& commandList);
	void DrawGeometryPass(CommandList& cmdList);
	void DrawLightingPass(CommandList& cmdList);
	void DrawJointDebug(CommandList& cmdList);
	void DrawBoneDebug(CommandList& cmdList);
	void DrawSkyboxPass(CommandList& cmdList);

	void DispatchEquiRectToCubemap(CommandList& cmdList);

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

private://Passes
	std::unique_ptr<DefaultPass> mDefaultPass;

	std::unique_ptr<GeometryPass> mGeometryPass;
	std::unique_ptr<LightingPass> mLightingPass;

	std::unique_ptr<SkyboxPass> mSkyboxPass;

private://Debug Passes
	std::unique_ptr<JointDebugPass> mJointDebugPass;

private://RTV & DSV SRV Resource
	FrameBufferResource mFrameResource;
	IBLResource mIBLResource;

	FrameBufferDescriptorIndex mDescIndex;
	IBLDescriptorIndex mIBLIndex;

private://CBV resource & allocation
	std::unique_ptr<CommonCB> mCommonCB;
	std::unique_ptr<LightCB> mLightCB;

private:
	std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<Object>> objects;
	std::unique_ptr<Object> mSkybox;

	std::unique_ptr<Camera> mCamera;
	POINT mLastMousePos;

	bool m_ContentLoaded = false;

	int blurCount = 5;
};


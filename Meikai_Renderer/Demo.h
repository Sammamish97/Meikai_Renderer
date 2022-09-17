#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "FrameBufferResource.h"
#include "DXApp.h"
#include "ConstantBuffers.h"
#include "DescriptorHeap.h"

#include "Texture.h"

struct Model;
struct Object;
class Camera;

class DefaultPass;
class GeometryPass;
class LightingPass;

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
	void CreateShader();

private:
	void CreateDescriptorHeaps();

	void CreateBufferResources();
	void CreateIBLResources();

	void CreateBufferDescriptors();
	void CreateIBLDescriptors();

	void DrawDefaultPass(CommandList& commandList);
	void DrawGeometryPass(CommandList& cmdList);
	void DrawLightingPass(CommandList& cmdList);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

private:
	void PreCalculateIBL();
	void ImageToCubeMap();
	void CalcDiffuseIrradiance();

private:
	void UpdatePassCB(const GameTimer& gt);
	void UpdateLightCB(const GameTimer& gt);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private://Passes
	std::unique_ptr<DefaultPass> mDefaultPass;

	std::unique_ptr<GeometryPass> mGeometryPass;
	std::unique_ptr<LightingPass> mLightingPass;

private://Descriptor heaps
	std::unique_ptr<DescriptorHeap> mDSVHeap;
	std::unique_ptr<DescriptorHeap> mRTVHeap;
	std::unique_ptr<DescriptorHeap> mCBVSRVUAVHeap;

private://RTV & DSV SRV Resource
	FrameBufferResource mFrameResource;
	IBLResource mIBLResource;

	FrameBufferDescriptorIndex mDescIndex;
	IBLDescriptorIndex mIBLIndex;

private://CBV resource & allocation
	std::unique_ptr<CommonCB> mCommonCB;
	UploadAllocation mCommonCBAllocation;

	std::unique_ptr<LightCB> mLightCB;
	UploadAllocation mLightAllocation;

	DefaultAllocation mTestDeafult;

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


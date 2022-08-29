#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "MaterialResource.h"
#include "DXApp.h"
#include "ConstantBuffers.h"
#include "DescriptorHeap.h"


struct Model;
struct Object;
struct Texture;
class Camera;

class DefaultPass;

class Demo : public DXApp
{
public:
	Demo(HINSTANCE hInstance);
	~Demo();

	bool Initialize() override;

protected:
	void OnResize() override;
	void Update(const GameTimer& gt) override;
	void Draw(const GameTimer& gt) override;

private:
	void BuildModels();
	void BuildFrameResource();
	void CreateShader();

private:
	void LoadContent();
	void CreateDescriptorHeaps();
	void CreateBufferResources();

	void CreateDepthStencilData();
	void CreateBufferDescriptors();

	void DrawGeometryPass();
	void DrawLightingPass();

	void Present();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

private:
	void UpdatePassCB(const GameTimer& gt);
	void UpdateLightCB(const GameTimer& gt);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private://Passes
	std::unique_ptr<DefaultPass> mDefaultPass;

private://Descriptor heaps
	std::unique_ptr<DescriptorHeap> mDSVHeap;
	std::unique_ptr<DescriptorHeap> mRTVHeap;
	std::unique_ptr<DescriptorHeap> mCBVSRVUAVHeap;

private://RTV & DSV SRV Resource
	std::unique_ptr<MaterialResource> mMatResource;

private://CBV resource & allocation
	std::unique_ptr<PassCB> mPassCB;
	UploadAllocation mPassAllocation;

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


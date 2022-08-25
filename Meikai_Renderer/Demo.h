#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "MaterialResource.h"
#include "DXApp.h"
#include "ConstantBuffers.h"


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
	void LoadContent();
	void CreateDepthStencilData();
	void BuildModels();

	void BuildFrameResource();
	void CreateShader();

	void DrawDefaultPass();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

private:
	void UpdatePassCB(const GameTimer& gt);
	void UpdateLightCB(const GameTimer& gt);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

private://Passes
	std::unique_ptr<DefaultPass> mDefaultPass;

private://RTV & SRV Resource
	std::unique_ptr<MaterialResource> mMatResource;

	std::unique_ptr<PassCB> mPassCB;
	UploadAllocation mPassAllocation;

	std::unique_ptr<LightCB> mLightCB;
	UploadAllocation mLightAllocation;

	DefaultAllocation mTestDeafult;

private://Descriptor heap for unbounded array
	ComPtr<ID3D12DescriptorHeap> mBindlessHeap;//0번은 constant, 뒤는 SRV들

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


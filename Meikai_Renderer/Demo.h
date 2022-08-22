#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include "MaterialResource.h"
#include "FrameResource.h"

#include "DXApp.h"

struct Model;
struct Object;
struct Texture;
class Camera;

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
	void BuildModels();

	void BuildFrameResource();
	void CreateShader();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

private:
	void UpdatePassCB(const GameTimer& gt);
	void UpdateLightCB(const GameTimer& gt);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private://RTV&SRV Resource
	std::unique_ptr <MaterialResource> mMatResource;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	std::unique_ptr<FrameResource> mFrameResource;

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


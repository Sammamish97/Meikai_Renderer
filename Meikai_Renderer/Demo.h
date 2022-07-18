#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include "FrameResource.h"

#include "DXApp.h"

struct Model;
struct Object;
class Camera;
class GeometryPass;
class LightingPass;
class SsaoPass;

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
	void DrawGeometry(const GameTimer& gt);
	void DrawSsao(const GameTimer& gt);
	void DrawLighting(const GameTimer& gt);

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

private:
	std::unique_ptr<GeometryPass> G_Pass;
	std::unique_ptr<LightingPass> L_Pass;
	std::unique_ptr<SsaoPass> S_Pass;

	std::unique_ptr<FrameResource> mFrameResource;

	std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<Object>> objects;

	std::unique_ptr<Camera> mCamera;
	POINT mLastMousePos;

	bool m_ContentLoaded = false;
};


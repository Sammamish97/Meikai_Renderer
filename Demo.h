#pragma once
#include <memory>
#include <vector>

#include "DXApp.h"

struct Model;
struct Object;
class Camera;
class GeometryPass;

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
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void LoadContent();
	void InitModel();
	void CreateDsvDescriptorHeap();
	void CreateShader();
	void CreateRootSignature();
	void CreatePSO();

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	std::unique_ptr<GeometryPass> G_Pass;

private:
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;

	// Root signature
	ComPtr<ID3D12RootSignature> m_RootSignature;

	// Pipeline state object.
	ComPtr<ID3D12PipelineState> m_PipelineState;

	std::unique_ptr<Camera> mCamera;
	POINT mLastMousePos;

	std::shared_ptr<Model> testModel = nullptr;
	std::vector<Object*> objects;

	bool m_ContentLoaded = false;
};


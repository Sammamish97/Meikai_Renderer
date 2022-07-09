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

private:
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void LoadContent();
	void BuildModels();
	void BuildGeometryPSO();
	void BuildGeometryRootSignature();
	void BuildFrameResource();

	void CreateGeometryRTV();
	void CreateDsvDescriptorHeap();
	void CreateShader();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetGeometryRtvCpuHandle(int index);

private:
	void UpdatePassCB(const GameTimer& gt);

protected:
	void OnMouseDown(WPARAM btnState, int x, int y) override;
	void OnMouseUp(WPARAM btnState, int x, int y) override;
	void OnMouseMove(WPARAM btnState, int x, int y) override;

private:
	std::unique_ptr<GeometryPass> G_Pass;
	std::unique_ptr<FrameResource> mFrameResource;

	ComPtr<ID3D12DescriptorHeap> mGeometryRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<Object>> objects;

	std::unique_ptr<Camera> mCamera;
	POINT mLastMousePos;

	bool m_ContentLoaded = false;
};


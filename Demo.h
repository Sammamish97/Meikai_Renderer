#pragma once
#include <DirectXMath.h>

#include "DXApp.h"

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
	
	D3D12_CPU_DESCRIPTOR_HANDLE Demo::DepthStencilView() const;

	void LoadContent();
	void CreateVertexResource();
	void CreateDsvDescriptorHeap();
	void CreateShader();
	void CreateRootSignature();
	void CreatePSO();


private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	// Vertex buffer for the cube.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// Index buffer for the cube.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;

	// Root signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

	// Pipeline state object.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

	float m_FoV = 45.f;

	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;


	bool m_ContentLoaded = false;
};


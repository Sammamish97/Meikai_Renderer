#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;

class DXApp;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 UV;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
};

struct Mesh
{
private:
	DXApp* m_dxApp = nullptr;

public:
	Mesh(std::vector<Vertex> input_vertices, std::vector<WORD> input_indices, DXApp* dxApp, ComPtr<ID3D12GraphicsCommandList2> commandList);
	ComPtr<ID3D12Resource> m_VertexBuffer;
	ComPtr<ID3D12Resource> m_IndexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	std::vector<Vertex> m_vertices;
	std::vector<WORD> m_indices;


private:
	void InitVB(ComPtr<ID3D12GraphicsCommandList2> commandList);
	void InitIB(ComPtr<ID3D12GraphicsCommandList2> commandList);
};


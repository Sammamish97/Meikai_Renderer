#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;

struct Model
{
	Model(const std::string& file_path);
	std::string name;

	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<WORD> indices;

	ComPtr<ID3D12Resource> m_VertexBuffer;
	ComPtr<ID3D12Resource> m_IndexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;


	bool LoadModel(const std::string& file_path);
};


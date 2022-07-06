#include "Mesh.h"
#include "DXApp.h"

Mesh::Mesh(std::vector<Vertex> input_vertices, std::vector<WORD> input_indices, DXApp* dxApp,
	ComPtr<ID3D12GraphicsCommandList2> commandList)
	:m_dxApp(dxApp), m_vertices(input_vertices), m_indices(input_indices)
{
	InitVB(commandList);
	InitIB(commandList);
}

void Mesh::InitVB(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	ComPtr<ID3D12Resource> stagingVB;
	m_dxApp->UpdateDefaultBufferResource(commandList, &m_VertexBuffer, &stagingVB,
		m_vertices.size(), sizeof(Vertex), m_vertices.data());

	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = m_vertices.size() * sizeof(Vertex);
	m_VertexBufferView.StrideInBytes = sizeof(Vertex);
}

void Mesh::InitIB(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	ComPtr<ID3D12Resource> stagingIB;
	m_dxApp->UpdateDefaultBufferResource(commandList, &m_IndexBuffer, &stagingIB,
		m_indices.size(), sizeof(WORD), m_indices.data());

	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes = m_indices.size() * sizeof(WORD);
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

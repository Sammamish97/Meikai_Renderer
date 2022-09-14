#include "Mesh.h"
#include "DXApp.h"

Mesh::Mesh(DXApp* dxApp, std::vector<Vertex> input_vertices, std::vector<WORD> input_indices, CommandList& commandList)
	:mApp(dxApp), mVertices(std::move(input_vertices)), mIndices(std::move(input_indices)), mVertexBuffer(dxApp), mIndexBuffer(dxApp)
{
	Init(commandList);
}

void Mesh::Init(CommandList& commandList)
{
	if(mVertices.size() >= USHRT_MAX)
	{
		throw std::exception("Too many vertices for 16-bit index buffer");
	}

	commandList.CopyVertexBuffer(mVertexBuffer, mVertices);
	commandList.CopyIndexBuffer(mIndexBuffer, mIndices);

	mIndexCount = static_cast<UINT>(mIndices.size());
}

void Mesh::Draw(CommandList& commandList)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, mVertexBuffer);
	commandList.SetIndexBuffer(mIndexBuffer);
	commandList.DrawIndexed(mIndexCount);
}

#include "Mesh.h"
#include "DXApp.h"

void Vertex::AddBoneData(int boneID, float weight)
{
	boneIDs[weightNum] = boneID;
	weights[weightNum] = weight;
	++weightNum;
}

Mesh::Mesh(DXApp* dxApp, std::vector<Vertex> input_vertices, std::vector<UINT> input_indices, CommandList& commandList)
	:mApp(dxApp), mVertices(std::move(input_vertices)), mIndices(std::move(input_indices)), mVertexBuffer(dxApp), mIndexBuffer(dxApp),
	mIndexCount(0), mBoneCount(0)
{
	Init(commandList);
}

void Mesh::Init(CommandList& commandList)
{
	if(mVertices.size() >= UINT_MAX)
	{
		throw std::exception("Too many vertices for 16-bit index buffer");
	}

	commandList.CopyVertexBuffer(mVertexBuffer, mVertices);
	mVertexBuffer.CreateVertexBufferView(mVertices.size(), sizeof(mVertices[0]));
	commandList.CopyIndexBuffer(mIndexBuffer, mIndices);
	mIndexBuffer.CreateViews(mIndices.size(), sizeof(mIndices[0]));

	mIndexCount = static_cast<UINT>(mIndices.size());
}

void Mesh::Draw(CommandList& commandList)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, mVertexBuffer);
	commandList.SetIndexBuffer(mIndexBuffer);
	commandList.DrawIndexed(mIndexCount);
}

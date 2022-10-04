#include "SkeletalMesh.h"
#include "DXApp.h"
#include "MathHelper.h"

SkeletalMesh::SkeletalMesh(DXApp* dxApp, const aiScene* aiPtr, std::vector<SkeletalVertex> input_vertices,
	std::vector<UINT> input_indices, CommandList& commandList)
	:mApp(dxApp), mScenePtr(aiPtr), mSkeletalVertices(std::move(input_vertices)), mIndices(std::move(input_indices)),
	mVertexBuffer(dxApp), mIndexBuffer(dxApp),
	mIndexCount(0)
{
	Init(commandList);
}

void SkeletalMesh::Init(CommandList& commandList)
{
	if (mSkeletalVertices.size() >= UINT_MAX)
	{
		throw std::exception("Too many vertices for 16-bit index buffer");
	}

	commandList.CopyVertexBuffer(mVertexBuffer, mSkeletalVertices);
	mVertexBuffer.CreateVertexBufferView(mSkeletalVertices.size(), sizeof(mSkeletalVertices[0]));
	commandList.CopyIndexBuffer(mIndexBuffer, mIndices);
	mIndexBuffer.CreateViews(mIndices.size(), sizeof(mIndices[0]));

	mIndexCount = static_cast<UINT>(mIndices.size());
}

void SkeletalMesh::Draw(CommandList& commandList)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, mVertexBuffer);
	commandList.SetIndexBuffer(mIndexBuffer);
	commandList.DrawIndexed(mIndexCount);
}

void SkeletalVertex::AddBoneData(int boneID, float weight)
{
	boneIDs[weightNum] = boneID;
	weights[weightNum] = weight;
	weightNum += 1;
}

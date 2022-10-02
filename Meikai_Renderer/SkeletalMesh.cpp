#include "SkeletalMesh.h"
#include "DXApp.h"
#include "MathHelper.h"
#include "Animation.h"

SkeletalMesh::SkeletalMesh(DXApp* dxApp, const aiScene* aiPtr, std::vector<SkeletalVertex> input_vertices,
	std::vector<UINT> input_indices, std::vector<BoneData> boneData, std::map<std::string, UINT> boneMap,
	CommandList& commandList)
	:mApp(dxApp), mScenePtr(aiPtr), mSkeletalVertices(std::move(input_vertices)), mIndices(std::move(input_indices)),
	mBoneData(std::move(boneData)), mBoneMap(std::move(boneMap)), mVertexBuffer(dxApp), mIndexBuffer(dxApp),
	mIndexCount(0)
{
	Init(commandList);
	mGlobalInverseTransform = mScenePtr->mRootNode->mTransformation.Inverse();
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

void SkeletalMesh::Draw(CommandList& commandList, float time ,std::shared_ptr<Animation> animation)
{
	std::vector<aiMatrix4x4> finalTransforms;
	GetBoneTransforms(time, animation,finalTransforms);
	commandList.SetGraphicsDynamicConstantBuffer(2, finalTransforms.size() * sizeof(aiMatrix4x4), finalTransforms.data());

	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, mVertexBuffer);
	commandList.SetIndexBuffer(mIndexBuffer);
	commandList.DrawIndexed(mIndexCount);
}

void SkeletalMesh::DrawDebugJoints(CommandList& commandList)
{
	auto vertexCount = mJointPositions.size();
	auto vertexSize = sizeof(mJointPositions[0]);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mJointPositions.data());
	commandList.Draw(vertexCount);
}

void SkeletalMesh::DrawDebugBones(CommandList& commandList)
{
	auto vertexCount = mBonePositions.size();
	auto vertexSize = sizeof(mBonePositions[0]);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mBonePositions.data());
	commandList.Draw(vertexCount);
}

void SkeletalMesh::ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode,
	std::shared_ptr<Animation> animation, aiMatrix4x4& parentTransform)
{
	std::string nodeName(pNode->mName.data);
	aiMatrix4x4 nodeTransformation(pNode->mTransformation);
	aiMatrix4x4 globalTransformation = parentTransform * nodeTransformation;

	aiNodeAnim* pNodeAnim = animation->FindNodeAnim(nodeName);
	if (pNodeAnim)
	{
		nodeTransformation = animation->CalcNodeTransformation(pNodeAnim, timeInSeconds);
	}

	if (mBoneMap.find(nodeName) != mBoneMap.end())
	{
		UINT boneIndex = mBoneMap[nodeName];
		mBoneData[boneIndex].finalMatrix = mGlobalInverseTransform * globalTransformation * mBoneData[boneIndex].offsetMatrix.Inverse();
	}

	for (UINT i = 0; i < pNode->mNumChildren; ++i)
	{
		ReadNodeHierarchy(timeInSeconds, pNode->mChildren[i], animation, globalTransformation);
	}
}

void SkeletalMesh::GetBoneTransforms(float timeInSeconds, std::shared_ptr<Animation> animation,
	std::vector<aiMatrix4x4>& Transforms)
{
	ReadNodeHierarchy(timeInSeconds, mScenePtr->mRootNode, animation, aiMatrix4x4());

	UINT boneDataSize = mBoneData.size();
	Transforms.resize(mBoneData.size());
	for (UINT i = 0; i < boneDataSize; ++i)
	{
		Transforms[i] = mBoneData[i].finalMatrix.Transpose();//Transpose because DX12 use row major. Therefore, transform.
	}
}

void SkeletalVertex::AddBoneData(int boneID, float weight)
{
	boneIDs[weightNum] = boneID;
	weights[weightNum] = weight;
	++weightNum;
}

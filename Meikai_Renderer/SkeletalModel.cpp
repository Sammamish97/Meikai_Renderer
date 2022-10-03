#include "SkeletalModel.h"
#include "MathHelper.h"
#include "Animation.h"

SkeletalModel::SkeletalModel(const std::string& file_path, DXApp* app, CommandList& commandList)
	:mApp(app)
{
	LoadModel(file_path, commandList);
}

void SkeletalModel::LoadModel(const std::string& file_path, CommandList& commandList)
{
    mImporter.SetPropertyBool(AI_CONFIG_FBX_USE_SKELETON_BONE_CONTAINER, true);

    pScene = mImporter.ReadFile(file_path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace );

    if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
    {
        assert("Fail to Load %s", file_path.c_str());
    }
    mGlobalInverseTransform = pScene->mRootNode->mTransformation.Inverse();
	name = file_path.substr(0, file_path.find_last_of('/'));
    ProcessNode(pScene->mRootNode, pScene, commandList);

    auto test1 = pScene->mNumSkeletons;
    auto test2 = pScene->mNumMeshes;
    auto skeleton1 = pScene->mSkeletons[0];
    auto skeleton2 = pScene->mSkeletons[1];
    auto mesh1 = pScene->mMeshes[0];
    auto mesh2 = pScene->mMeshes[1];
    auto bone = mesh1->mBones;
    std::vector<std::string> boneName1;
    std::vector<std::string> boneName2;

    for(int i = 0; i < mesh1->mNumBones; ++i)
    {
        boneName1.push_back(mesh1->mBones[i]->mName.data);
    }
    for (int i = 0; i < mesh2->mNumBones; ++i)
    {
        boneName2.push_back(mesh2->mBones[i]->mName.data);
    }

    for(int i = 0; i < mesh1->mNumBones; ++i)
    {
        if(boneName1[i] != boneName2[i])
        {
            int j = 0;
            ++j;
        }
    }

    mBoneMap;
    mBoneData;
}

void SkeletalModel::ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        mMeshes.push_back(ProcessMesh(mesh, scene, commandList));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, commandList);
    }
}

SkeletalMesh SkeletalModel::ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList)
{
    std::vector<SkeletalVertex> vertices;
    std::vector<UINT> indices;

    LoadVertices(mesh, vertices);
    if (mesh->HasFaces())
    {
        LoadIndices(mesh, indices);
    }
    if (mesh->HasBones())
    {
        LoadBones(mesh, vertices);
    }

    return SkeletalMesh(mApp, scene, vertices, indices, commandList);
}

void SkeletalModel::LoadVertices(aiMesh* mesh, std::vector<SkeletalVertex>& vertices)
{
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        SkeletalVertex vertex;
        XMFLOAT3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.position = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            XMFLOAT2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.UV = vec;
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.tangent = vector;
            // bitangent
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.biTangent = vector;
        }
        else
        {
            vertex.UV = XMFLOAT2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }
}

void SkeletalModel::LoadIndices(aiMesh* mesh, std::vector<UINT>& indices)
{
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            auto test = face.mIndices[j];
            indices.push_back(face.mIndices[j]);
        }
    }
}

void SkeletalModel::LoadBones(aiMesh* mesh, std::vector<SkeletalVertex>& vertices)
{
    for(int i = 0; i < mesh->mNumBones; ++i)
    {
        UINT boneIndex = 0;
        std::string boneName(mesh->mBones[i]->mName.data);

        if(mBoneMap.find(boneName) == mBoneMap.end())
        {
            boneIndex = mTotalNumBones;
            mTotalNumBones++;
            BoneData bi;
            mBoneData.push_back(bi);
        }
        else
        {
            boneIndex = mBoneMap[boneName];
        }

        mBoneMap[boneName] = boneIndex;
        mBoneData[boneIndex].offsetMatrix = mesh->mBones[i]->mOffsetMatrix.Inverse();
        //Currently, mOffset is bone -> local. Therefore, inverse it.

        for(int j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
        {
            UINT vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
            float weight = mesh->mBones[i]->mWeights[j].mWeight;
            vertices[vertexID].AddBoneData(boneIndex, weight);
        }
    }
}

void SkeletalModel::ReadNodeHierarchy(float timeInSeconds, const aiNode* pNode, std::shared_ptr<Animation> animation,
	aiMatrix4x4& parentTransform, aiVector3t<float> parentPos)
{
    std::string nodeName(pNode->mName.data);
    aiMatrix4x4 nodeTransformation(pNode->mTransformation);//Parent -> child matrix

    aiNodeAnim* pNodeAnim = animation->FindNodeAnim(nodeName);
    if (pNodeAnim)
    {
        //nodeTransformation = animation->CalcNodeTransformation(pNodeAnim, timeInSeconds);
    }

    aiMatrix4x4 globalTransformation = parentTransform * nodeTransformation;

    if (mBoneMap.find(nodeName) != mBoneMap.end())
    {
        UINT boneIndex = mBoneMap[nodeName];
        aiMatrix4x4 finalMat = globalTransformation * mBoneData[boneIndex].offsetMatrix;
        mBoneData[boneIndex].finalMatrix = finalMat;

        aiQuaterniont<float> rotation;
        aiVector3t<float> position;

        globalTransformation.DecomposeNoScaling(rotation, position);
        mJointPositions.push_back(MathHelper::AiVecToDxVec(position));
        mBonePositions.push_back(MathHelper::AiVecToDxVec(parentPos));
        mBonePositions.push_back(MathHelper::AiVecToDxVec(position));

        parentPos = position;
    }
    for (UINT i = 0; i < pNode->mNumChildren; ++i)
    {
        ReadNodeHierarchy(timeInSeconds, pNode->mChildren[i], animation, globalTransformation, parentPos);
    }
}

void SkeletalModel::GetBoneTransforms(float timeInSeconds, std::shared_ptr<Animation> animation,
	std::vector<aiMatrix4x4>& Transforms)
{
    ReadNodeHierarchy(timeInSeconds, pScene->mRootNode, animation, aiMatrix4x4(), aiVector3t(0.f));

    UINT boneDataSize = mBoneData.size();
    Transforms.resize(mBoneData.size());
    for (UINT i = 0; i < boneDataSize; ++i)
    {
        Transforms[i] = mBoneData[i].finalMatrix;
    }
}

void SkeletalModel::Draw(CommandList& commandList, float time, std::shared_ptr<Animation> animation)
{
    mFinalTransforms.clear();
    mJointPositions.clear();
    mBonePositions.clear();

    GetBoneTransforms(time, animation, mFinalTransforms);
    commandList.SetGraphicsDynamicConstantBuffer(2, mFinalTransforms.size() * sizeof(aiMatrix4x4), mFinalTransforms.data());
    for (auto& mesh : mMeshes)
    {
        mesh.Draw(commandList, 1.f, animation);
    }
}

void SkeletalModel::DrawDebugJoints(CommandList& commandList)
{
    auto vertexCount = mJointPositions.size();
    auto vertexSize = sizeof(mJointPositions[0]);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mJointPositions.data());
    commandList.Draw(vertexCount);
}

void SkeletalModel::DrawDebugBones(CommandList& commandList)
{
    auto vertexCount = mBonePositions.size();
    auto vertexSize = sizeof(mBonePositions[0]);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mBonePositions.data());
    commandList.Draw(vertexCount);
}
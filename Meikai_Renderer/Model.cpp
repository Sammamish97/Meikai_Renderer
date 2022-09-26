#include "Model.h"
#include "DXApp.h"
#include "CommandList.h"
#include "MathHelper.h"
#include "Animation.h"

Model::Model(const std::string& file_path, DXApp* app, CommandList& commandList)
	:mApp(app)
{
    LoadModel(file_path, commandList);
}

void Model::LoadModel(const std::string& file_path, CommandList& commandList)
{
    pScene = mImporter.ReadFile(file_path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode)
    {
        assert("Fail to Load %s", file_path.c_str());
    }

    name = file_path.substr(0, file_path.find_last_of('/'));
    ProcessNode(pScene->mRootNode, pScene, commandList);
}


void Model::ProcessNode(aiNode* node, const aiScene* scene, CommandList& commandList)
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

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, CommandList& commandList)
{
    std::vector<Vertex> vertices;
    std::vector<UINT> indices;
    //std::vector<textures>

    LoadVertices(mesh, vertices);
    if(mesh->HasFaces())
    {
        LoadIndices(mesh, indices);
    }
    if (mesh->HasBones())
    {
        LoadBones(mesh, vertices);
    }
   
    return Mesh(mApp, vertices, indices, commandList);
}

void Model::InitJointVertexBuffer(CommandList& commandList)
{
    
}

void Model::InitBoneVertexBuffer(CommandList& commandList)
{
}

void Model::UpdateGPUJointPosition(CommandList& commandList)
{
}

void Model::UpdateGPUBonePosition(CommandList& commandList)
{
}

void Model::DrawDebugJoints(CommandList& commandList)
{
    auto vertexCount = mJointPositions.size();
    auto vertexSize = sizeof(mJointPositions[0]);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mJointPositions.data());
    commandList.Draw(vertexCount);
}

void Model::DrawDebugBones(CommandList& commandList)
{
    auto vertexCount = mBonePositions.size();
    auto vertexSize = sizeof(mBonePositions[0]);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mBonePositions.data());
    commandList.Draw(vertexCount);
}

void Model::LoadVertices(aiMesh* mesh, std::vector<Vertex>& vertices)
{
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
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

void Model::LoadIndices(aiMesh* mesh, std::vector<UINT>& indices)
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

void Model::LoadBones(aiMesh* mesh, std::vector<Vertex>& vertices)
{
    int boneCount = 0;
    for (UINT boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
    {
        BoneData boneData;
        aiBone* bone = mesh->mBones[boneIdx];
        boneData.offsetMatrix = bone->mOffsetMatrix;//Offset set bone to mesh.
        std::string boneName = mesh->mBones[boneIdx]->mName.data;

        UINT weightNum = bone->mNumWeights;
        for (UINT weightIdx = 0; weightIdx < weightNum; ++weightIdx)
        {
            auto vertexBoneData = bone->mWeights[weightIdx];
            UINT vertexID = vertexBoneData.mVertexId;//TODO: Notify base index.
            float weight = vertexBoneData.mWeight;
            vertices[vertexID].AddBoneData(boneCount, weight);
        }
        mBoneMap[boneName] = boneCount;
        mBoneData.push_back(boneData);
        boneCount++;
    }

    ExtractJoint();
    ExtractBone();
}

void Model::ExtractJoint()
{
    mJointPositions.clear();
    for(const auto& boneData : mBoneData)
    {
        aiQuaterniont<float> rotation;
        aiVector3t<float> position;
        auto localToBone = boneData.offsetMatrix;
        localToBone.Inverse().DecomposeNoScaling(rotation, position);
        mJointPositions.push_back(MathHelper::AiVecToDxVec(position));
    }
}

void Model::ExtractBone()
{
    mBonePositions.clear();
    aiVector3t<float> localOrigin{ 0, 0, 0 };
    ExtractBoneRecursive(pScene->mRootNode, localOrigin);
}

void Model::ExtractBoneRecursive(const aiNode* pNode, aiVector3t<float> parentPos)
{
    if(mBonePositions.size() == 14)
    {
        pNode = pNode;
    }
    
    std::string boneName = pNode->mName.data;
    if(mBoneMap.find(boneName) != mBoneMap.end())
    {
        UINT boneIndex = mBoneMap.at(boneName);
        aiMatrix4x4 offsetMatrix = mBoneData[boneIndex].offsetMatrix;
        auto localToBone = offsetMatrix;
        localToBone.Inverse();
        aiQuaterniont<float> rotation;//Don't use.
        aiVector3t<float> position;
        localToBone.DecomposeNoScaling(rotation, position);

        mBonePositions.push_back(MathHelper::AiVecToDxVec(parentPos));
        mBonePositions.push_back(MathHelper::AiVecToDxVec(position));

        parentPos = position;
    }
    for (UINT i = 0; i < pNode->mNumChildren; ++i)
    {
        ExtractBoneRecursive(pNode->mChildren[i], parentPos);
    }
}

#include "Animator.h"
Animator::Animator(std::shared_ptr<Animation> animation)
{
	m_CurrentTime = 0.0;
	m_CurrentAnimation = animation;

	m_GlobalInverse = animation->GetRootNode().transformation;

	m_FinalBoneMatrices.reserve(100);
	for (int i = 0; i < 100; i++)
		m_FinalBoneMatrices.push_back(aiMatrix4x4());
}

void Animator::UpdateAnimation(float dt, std::vector<aiVector3t<float>>& jointPosition, std::vector<aiVector3t<float>>& bonePosition)
{
	m_DeltaTime = dt;
	if (m_CurrentAnimation)
	{
		m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
		m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
		CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), aiMatrix4x4(), aiVector3t<float>(), jointPosition, bonePosition);
	}
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation)
{
	m_CurrentAnimation = pAnimation;
	m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AssimpNodeData* node, aiMatrix4x4 parentTransform, aiVector3t<float> parentPos,
	std::vector<aiVector3t<float>>& jointPosition, std::vector<aiVector3t<float>>& bonePosition)
{
	std::string nodeName = node->name;
	aiMatrix4x4 nodeTransform = node->transformation;

	Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

	if (Bone)
	{
		Bone->Update(m_CurrentTime);
		nodeTransform = Bone->GetLocalTransform();
	}

	aiMatrix4x4 globalTransformation = parentTransform * nodeTransform;

	auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		int index = boneInfoMap[nodeName].id;
		aiMatrix4x4 offset = boneInfoMap[nodeName].offset;

		aiQuaterniont<float> rotation;
		aiVector3t<float> position;
		globalTransformation.DecomposeNoScaling(rotation, position);
		jointPosition.push_back(position);
		bonePosition.push_back(parentPos);
		bonePosition.push_back(position);

		m_FinalBoneMatrices[index] = globalTransformation * offset;

		parentPos = position;
	}

	for (int i = 0; i < node->childrenCount; i++)
		CalculateBoneTransform(&node->children[i], globalTransformation, parentPos, jointPosition, bonePosition);
}

std::vector<aiMatrix4x4> Animator::GetFinalBoneMatrices()
{
	return m_FinalBoneMatrices;
}
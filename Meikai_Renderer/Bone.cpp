#include "Bone.h"
Bone::Bone(const std::string& name, int ID, const aiNodeAnim* channel)
	:
	m_Name(name),
	m_ID(ID),
	m_LocalTransform()
{
	m_NumPositions = channel->mNumPositionKeys;

	for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
	{
		aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
		float timeStamp = channel->mPositionKeys[positionIndex].mTime;
		KeyPosition data;
		data.position = aiPosition;
		data.timeStamp = timeStamp;
		m_Positions.push_back(data);
	}

	m_NumRotations = channel->mNumRotationKeys;
	for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
		float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
		KeyRotation data;
		data.orientation = aiOrientation;
		data.timeStamp = timeStamp;
		m_Rotations.push_back(data);
	}

	m_NumScalings = channel->mNumScalingKeys;
	for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
	{
		aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
		float timeStamp = channel->mScalingKeys[keyIndex].mTime;
		KeyScale data;
		data.scale = scale;
		data.timeStamp = timeStamp;
		m_Scales.push_back(data);
	}
}

void Bone::Update(float animationTime)
{
	aiMatrix4x4 translation = InterpolatePosition(animationTime);
	aiMatrix4x4 rotation = InterpolateRotation(animationTime);
	aiMatrix4x4 scale = InterpolateScaling(animationTime);
	m_LocalTransform = translation * rotation * scale;
}

int Bone::GetPositionIndex(float animationTime)
{
	for (int index = 0; index < m_NumPositions - 1; ++index)
	{
		if (animationTime < m_Positions[index + 1].timeStamp)
			return index;
	}
	assert(0);
}

int Bone::GetRotationIndex(float animationTime)
{
	for (int index = 0; index < m_NumRotations - 1; ++index)
	{
		if (animationTime < m_Rotations[index + 1].timeStamp)
			return index;
	}
	assert(0);
}

int Bone::GetScaleIndex(float animationTime)
{
	for (int index = 0; index < m_NumScalings - 1; ++index)
	{
		if (animationTime < m_Scales[index + 1].timeStamp)
			return index;
	}
	assert(0);
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	float scaleFactor = 0.0f;
	float midWayLength = animationTime - lastTimeStamp;
	float framesDiff = nextTimeStamp - lastTimeStamp;
	scaleFactor = midWayLength / framesDiff;
	return scaleFactor;
}

aiMatrix4x4 Bone::InterpolatePosition(float animationTime)
{
	if (1 == m_NumPositions)
		return aiMatrix4x4::Translation(m_Positions[0].position, aiMatrix4x4());

	int p0Index = GetPositionIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
		m_Positions[p1Index].timeStamp, animationTime);

	const aiVector3D& Start = m_Positions[p0Index].position;
	const aiVector3D& End = m_Positions[p1Index].position;
	aiVector3D Delta = End - Start;
	aiVector3D finalPosition = Start + scaleFactor * Delta;

	return aiMatrix4x4::Translation(finalPosition, aiMatrix4x4());
}

aiMatrix4x4 Bone::InterpolateRotation(float animationTime)
{
	if (1 == m_NumRotations)
	{
		auto rotation = m_Rotations[0].orientation.Normalize();
		return aiMatrix4x4(rotation.GetMatrix());
	}

	int p0Index = GetRotationIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp,
		m_Rotations[p1Index].timeStamp, animationTime);
	aiQuaterniont<float> finalRotation;
	aiQuaternion::Interpolate(finalRotation, m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation
		, scaleFactor);
	finalRotation = finalRotation.Normalize();
	return aiMatrix4x4(finalRotation.GetMatrix());

}

aiMatrix4x4 Bone::InterpolateScaling(float animationTime)
{
	if (1 == m_NumScalings)
		return aiMatrix4x4::Scaling(m_Scales[0].scale, aiMatrix4x4());

	int p0Index = GetScaleIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
		m_Scales[p1Index].timeStamp, animationTime);

	const aiVector3D& Start = m_Scales[p0Index].scale;
	const aiVector3D& End = m_Scales[p1Index].scale;
	aiVector3D Delta = End - Start;
	aiVector3D finalScale = Start + scaleFactor * Delta;
	return aiMatrix4x4::Scaling(finalScale, aiMatrix4x4());
}
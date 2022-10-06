#pragma once
#include <cassert>
#include <vector>
#include <assimp/scene.h>
#include <list>

struct KeyPosition
{
	aiVector3t<float> position;
	float timeStamp;
};

struct KeyRotation
{
	aiQuaterniont<float> orientation;
	float timeStamp;
};

struct KeyScale
{
	aiVector3t<float> scale;
	float timeStamp;
};

/**
 * @brief Class for represent each bone node.
 * @detail Each bone cache every key frame data of arbitrary animation.
 */
class Bone
{
public:
	Bone(const std::string& name, int ID, const aiNodeAnim* channel);
	void Update(float animationTime);

	aiMatrix4x4 GetLocalTransform() { return m_LocalTransform; }
	std::string GetBoneName() const { return m_Name; }
	int GetBoneID() { return m_ID; }

	int GetPositionIndex(float animationTime);
	int GetRotationIndex(float animationTime);
	int GetScaleIndex(float animationTime);

private:

	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

	/**
	 * @brief Function for interpolating position.
	 * @detail Use Lerp for interpolating between frame.
	*/
	aiMatrix4x4 InterpolatePosition(float animationTime);

	/**
	 * @brief Function for interpolating rotation.
	 * @detail Use quaternion Slerp for interpolating between frame.
	*/
	aiMatrix4x4 InterpolateRotation(float animationTime);

	/**
	 * @brief Function for interpolating scale.
	 * @detail Use Lerp for interpolating between frame.
	*/
	aiMatrix4x4 InterpolateScaling(float animationTime);

	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;
	int m_NumPositions;
	int m_NumRotations;
	int m_NumScalings;

	aiMatrix4x4 m_LocalTransform;
	std::string m_Name;
	int m_ID;
};


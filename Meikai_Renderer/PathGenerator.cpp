#include "PathGenerator.h"
#include "CommandList.h"
#include "MathHelper.h"
#include "Model.h"

PathGenerator::PathGenerator(std::shared_ptr<Model> controlPointModel)
{
	mControlPointModel = controlPointModel;
	mSlice = 100;
	mTimeAccumulating = 0.f;
	float scale = 3.f;

	auto test0 = XMFLOAT3(0, 0, scale * 1.f);
	auto test1 = XMFLOAT3(-scale, 0, scale);
	auto test2 = XMFLOAT3(-scale * 2.f, 0, 0);
	auto test3 = XMFLOAT3(-scale, 0, -scale);
	auto test4 = XMFLOAT3(0, 0, -scale * 2.f);
	auto test5 = XMFLOAT3(scale, 0, -scale);
	auto test6 = XMFLOAT3(scale * 2.f, 0, 0);
	auto test7 = XMFLOAT3(scale, 0, scale);

	mControlPoints.push_back(XMLoadFloat3(&test0));
	mControlPoints.push_back(XMLoadFloat3(&test1));
	mControlPoints.push_back(XMLoadFloat3(&test2));
	mControlPoints.push_back(XMLoadFloat3(&test3));
	mControlPoints.push_back(XMLoadFloat3(&test4));
	mControlPoints.push_back(XMLoadFloat3(&test5));
	mControlPoints.push_back(XMLoadFloat3(&test6));
	mControlPoints.push_back(XMLoadFloat3(&test7));
	mControlPoints.push_back(XMLoadFloat3(&test0));

	CalcSubPoints();
	BuildFunctions();
	BuildAdaptiveTable(0.000001f);
	GetPointStrip();
}

void PathGenerator::Update(GameTimer timer)
{
	mTimeAccumulating += timer.DeltaTime() / 100.f;
	if(mTimeAccumulating > 1)
	{
		mTimeAccumulating = 0;
	}
	ArcLengthToPosition(DistanceTimeFunction(mTimeAccumulating));
}

void PathGenerator::DrawPaths(CommandList& commandList)
{
	auto vertexCount = mPathLines.size();
	auto vertexSize = sizeof(mPathLines[0]);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mPathLines.data());
	commandList.Draw(vertexCount);
}

void PathGenerator::DrawControlPoints(CommandList& commandList)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	float pointScale = 0.05f;
	XMMATRIX scale = XMMatrixScaling(pointScale, pointScale, pointScale);

	for (auto element : mControlPoints)
	{
		XMFLOAT3 position;
		XMStoreFloat3(&position, element);
		XMMATRIX translation = XMMatrixTranspose(scale * XMMatrixTranslation(position.x, position.y, position.z));
		XMMATRIX result;
		for (auto mesh : mControlPointModel->mMeshes)
		{
			commandList.SetGraphics32BitConstants(0, translation);
			mesh.Draw(commandList);
		}
	}
}

void PathGenerator::GetPointStrip()
{
	mPathLines.clear();
	float slice = static_cast<float>(mSlice);
	for(const auto& curveFunc : mBezierEquations)
	{
		for(float i = 0.f; i < 1; i += (1.f / slice))
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, curveFunc(i));
			mPathLines.push_back(value);
		}
	}
}

void PathGenerator::BuildFunctions()
{
	XMVECTOR controlPointsSet[4];
	for(int i = 0; i < mControlPoints.size() - 1; ++i)
	{
		controlPointsSet[0] = mControlPoints[i];
		for(int j = 0; j < 2; ++j)
		{
			controlPointsSet[j + 1] = mSubPoints[2 * i + j];
		}
		controlPointsSet[3] = mControlPoints[i + 1];
		auto curveEquation = [=](float u) -> XMVECTOR
		{
			XMVECTOR result;
			float u3 = u * u * u;
			float u2 = u * u;

			auto first = -u3 + 3.f * u2 - 3.f * u + 1.f;
			auto second = 3.f * u3 - 6.f * u2 + 3.f * u;
			auto third = -3.f * u3 + 3.f * u2;
			auto fourth = u3;

			result = first * controlPointsSet[0] + second * controlPointsSet[1] +
				third * controlPointsSet[2] + fourth * controlPointsSet[3];

			return result;
		};
		mBezierEquations.push_back(curveEquation);
	}
}

void PathGenerator::CalcSubPoints()
{
	auto length = mControlPoints.size();
	//a_0
	mSubPoints.push_back(CalcA(mControlPoints[length - 1], mControlPoints[0], mControlPoints[1]));

	//b_1
	mSubPoints.push_back(CalcB(mControlPoints[0], mControlPoints[1], mControlPoints[2]));

	for(int i = 3; i < mControlPoints.size(); ++i)
	{
		//a_(i-1)
		mSubPoints.push_back(CalcA(mControlPoints[i - 3], mControlPoints[i - 2], mControlPoints[i - 1]));

		//b_(i)
		mSubPoints.push_back(CalcB(mControlPoints[i - 2], mControlPoints[i - 1], mControlPoints[i]));
	}

	//a_(n-1)
	//2,3,4
	mSubPoints.push_back(CalcA(mControlPoints[length - 3], mControlPoints[length - 2], mControlPoints[length - 1]));

	//b_(n)
	//3,4,5(p[0])
	mSubPoints.push_back(CalcB(mControlPoints[length - 2], mControlPoints[length - 1], mControlPoints[0]));
}

XMVECTOR PathGenerator::CalcA(XMVECTOR p_0, XMVECTOR p_1, XMVECTOR p_2)
{
	return p_1 + (p_2 - p_0) * 0.5f;
}

XMVECTOR PathGenerator::CalcB(XMVECTOR p_0, XMVECTOR p_1, XMVECTOR p_2)
{
	return p_1 - (p_2 - p_0) * 0.5f;
}

int PathGenerator::GetBezierIndex(float globalU)
{
	return std::clamp(floorf(mBezierEquations.size() * globalU), 0.f, mBezierEquations.size()-1.f);
}

float PathGenerator::DenormalizeU(float globalU, int index)
{
	return globalU * mBezierEquations.size() - index;
}

XMFLOAT3 PathGenerator::GetPointDistances(float u_a, float u_b, float u_m)
{
	int a_funcIndex = GetBezierIndex(u_a);
	int b_funcIndex = GetBezierIndex(u_b);
	int m_funcIndex = GetBezierIndex(u_m);

	float a_localU = DenormalizeU(u_a, a_funcIndex);
	float b_localU = DenormalizeU(u_b, b_funcIndex);
	float m_localU = DenormalizeU(u_m, m_funcIndex);

	auto a_pos = mBezierEquations[a_funcIndex](a_localU);
	auto m_pos = mBezierEquations[m_funcIndex](m_localU);
	auto b_pos = mBezierEquations[b_funcIndex](b_localU);

	XMVECTOR s_am = XMVector3Length(m_pos - a_pos);
	XMVECTOR s_mb = XMVector3Length(b_pos - m_pos);
	XMVECTOR s_ab = XMVector3Length(b_pos - a_pos);
	XMFLOAT3 lengthOutputReadable;

	XMStoreFloat3(&lengthOutputReadable, s_am);
	float am_length = lengthOutputReadable.x;

	XMStoreFloat3(&lengthOutputReadable, s_mb);
	float mb_length = lengthOutputReadable.x;

	XMStoreFloat3(&lengthOutputReadable, s_ab);
	float ab_length = lengthOutputReadable.x;

	return XMFLOAT3(am_length, mb_length, ab_length);
}

void PathGenerator::BuildAdaptiveTable(float threshHold)
{
	std::list<std::pair<float, float>> segments;
	mParamArcLengthMap[0.f] = 0.f;
	int preSegmentAmount = 10;
	float preSegmentSlice = 1.f / preSegmentAmount;
	for(int i = 0; i < preSegmentAmount; ++i)
	{
		segments.push_back({i * preSegmentSlice, (i+1) * preSegmentSlice });
	}
	auto i = segments.begin();
	while(segments.empty() == false)
	{
		float u_a = i->first;
		float u_b = i->second;
		float u_m = (u_a + u_b) * 0.5f;

		auto distances = GetPointDistances(u_a, u_b, u_m);//Return order: am -> mb -> ab
		float am_length = distances.x;
		float mb_length = distances.y;
		float ab_length = distances.z;

		if (am_length + mb_length - ab_length < threshHold)
		{
			//Case1: Complete
			float firstHalfLength = mParamArcLengthMap[u_a] + am_length;
			mParamArcLengthMap[u_m] = firstHalfLength;
			mParamArcLengthMap[u_b] = firstHalfLength + mb_length;
			segments.remove(*i);
		}
		else
		{
			//Case2: Divide and Repeat
			segments.remove(*i);
			std::pair firsthalf = {u_a, u_m};
			std::pair secondhalf = {u_m, u_b};
			segments.push_front(secondhalf);
			segments.push_front(firsthalf);
		}
		i = segments.begin();
	}
	float lastArcLength = (--mParamArcLengthMap.end())->second;
	for (auto& element : mParamArcLengthMap)
	{
		element.second /= lastArcLength;
	}
	for (auto element : mParamArcLengthMap)
	{
		mArcLengthParamMap[element.second] = element.first;
	}
}

float PathGenerator::DistanceTimeFunction(float time)
{
	//return (sin(time * MathHelper::Pi - MathHelper::Pi / 2.f) + 1) * 0.5f;
	return time;
}

void PathGenerator::ArcLengthToPosition(float arcLength)
{
	static std::vector<int> test;
	auto highUitor = mArcLengthParamMap.lower_bound(arcLength);
	auto lowUitor = highUitor; --lowUitor;
	if(highUitor == mArcLengthParamMap.begin())
	{
		mCurrentPosition = mBezierEquations[0](0);
		return;
	}
	float highU = highUitor->second;
	float lowU = lowUitor->second;

	auto highS = mParamArcLengthMap[highU];
	auto lowS = mParamArcLengthMap[lowU];

	float interpolateArcLength = (arcLength - lowS) / (highS - lowS);
	float interpolatedU = (highU - lowU) * interpolateArcLength + lowU;

	int segmentNum = mBezierEquations.size();

	int equationIndex = std::clamp(floorf(interpolatedU * segmentNum), 0.f, (float)(segmentNum -1));
	float denormalizeU = interpolatedU * segmentNum - equationIndex;
	auto currentPosition = mBezierEquations[equationIndex](denormalizeU);

	int nextIndex = std::clamp(floorf(highU * segmentNum), 0.f, (float)(segmentNum - 1));
	float nextDenormalizeU = highU * segmentNum - nextIndex;
	auto nextPosition = mBezierEquations[nextIndex](nextDenormalizeU);

	mCurrentFrameRotation = XMVector3Normalize(nextPosition - currentPosition);
	mCurrentPosition = currentPosition;
}

XMVECTOR PathGenerator::GetDirection()
{
	return mCurrentFrameRotation;
}

XMVECTOR PathGenerator::GetPosition()
{
	return mCurrentPosition;  
}

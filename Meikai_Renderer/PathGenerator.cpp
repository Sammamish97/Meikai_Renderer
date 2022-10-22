#include "PathGenerator.h"
#include "CommandList.h"
#include "MathHelper.h"

PathGenerator::PathGenerator()
{
	mSlice = 100;
	mTimeAccumulating = 0.f;

	auto test1 = XMFLOAT3(5, 0, 5);
	auto test2 = XMFLOAT3(-5, 0, 5);
	auto test3 = XMFLOAT3(-5, 0, -5);
	auto test4 = XMFLOAT3(5, 0, -5);
	auto test5 = XMFLOAT3(5, 0, 5);


	mControlPoints.push_back(XMLoadFloat3(&test1));
	mControlPoints.push_back(XMLoadFloat3(&test2));
	mControlPoints.push_back(XMLoadFloat3(&test3));
	mControlPoints.push_back(XMLoadFloat3(&test4));
	mControlPoints.push_back(XMLoadFloat3(&test5));

	ClacSubPoints();
	BuildFunctions();
	GetPointStrip();
	BuildArcTable();
}

XMVECTOR PathGenerator::Update(GameTimer timer)
{
	mTimeAccumulating += timer.DeltaTime() / 10.f;
	if(mTimeAccumulating > 1)
	{
		mTimeAccumulating = 0;
	}
	return ArcLengthToPosition(mTimeAccumulating);
}

void PathGenerator::Draw(CommandList& commandList)
{
	auto vertexCount = mPathLines.size();
	auto vertexSize = sizeof(mPathLines[0]);
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	commandList.SetDynamicVertexBuffer(0, vertexCount, vertexSize, mPathLines.data());
	commandList.Draw(vertexCount);
}

void PathGenerator::GetPointStrip()
{
	mPathLines.clear();
	float slice = static_cast<float>(mSlice);
	for(const auto& curveFunc : mBezierEquations)
	{
		for(float i = 0.f; i < 1; i += 1.f / slice)
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

void PathGenerator::ClacSubPoints()
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

void PathGenerator::BuildArcTable()
{
	float delta_u = 1.f / mSlice;
	float accumulatingU = 0;
	XMFLOAT3 lengthOutputReadable;
	mArcArray.push_back(EquationData(0.f, 0.f));
	for (int i = 0; i < mBezierEquations.size(); ++i)
	{
		float normalizedU = 0.f;
		for (float j = 0; j <= 1.f; j += delta_u)
		{
			accumulatingU += delta_u;
			normalizedU += delta_u;
			auto lengthOutput = XMVector3Length(mBezierEquations[i](normalizedU) - mBezierEquations[i](j));
			XMStoreFloat3(&lengthOutputReadable, lengthOutput);
			float currentSegLength = lengthOutputReadable.x;
			float accumulatedLength = mArcArray[mArcArray.size() - 1].ArcLength + currentSegLength;
			mArcArray.push_back(EquationData(accumulatedLength, accumulatingU));
		}
	}
	int arcArrayLenght = mArcArray.size();
	float totalU = mArcArray[arcArrayLenght - 1].U;
	float totalArcLength = mArcArray[arcArrayLenght - 1].ArcLength;

	for(int i = 0; i < arcArrayLenght; ++i)
	{
		mArcArray[i].ArcLength /= totalArcLength;
		mArcArray[i].U /= totalU;
		mArcMap[mArcArray[i].ArcLength] = ArcMapData(i, mArcArray[i].U);
	}
}

float PathGenerator::DistanceTimeFunction(float time)
{
	//return (sin(time * MathHelper::Pi - MathHelper::Pi / 2.f) + 1) * 0.5f;
	return time;
}

XMVECTOR PathGenerator::ArcLengthToPosition(float arcLength)
{
	static std::vector<int> test;
	auto lowOne = --mArcMap.lower_bound(arcLength);
	if(lowOne == mArcMap.end())
	{
		lowOne = mArcMap.begin();
	}
	ArcMapData lowOneMap = lowOne->second;

	EquationData lowOneData = mArcArray[lowOneMap.ArrayIndex];
	EquationData highOneData = mArcArray[lowOneMap.ArrayIndex + 1];

	float lowOneArclength = lowOneData.ArcLength;
	float lowOneU = lowOneData.U;

	float highOneArclength = highOneData.ArcLength;
	float highOneU = highOneData.U;

	float interpolateArcLength = (arcLength - lowOneArclength) / (highOneArclength - lowOneArclength);
	float interpolatedU = (highOneU - lowOneU) * interpolateArcLength + lowOneU;

	int segmentNum = mBezierEquations.size();
	int equationIndex = floorf(interpolatedU * segmentNum);
	float denormalizeU = interpolatedU * segmentNum - equationIndex;
	
	return mBezierEquations[equationIndex](denormalizeU);
}

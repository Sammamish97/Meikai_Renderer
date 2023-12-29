#pragma once
#include <vector>
#include <DirectXMath.h>
#include <functional>
#include <map>
#include <memory>

#include "GameTimer.h"

using namespace DirectX;
class CommandList;
class Model;
class PathGenerator
{
public:
	PathGenerator(std::shared_ptr<Model> controlPointModel);
	float Update(GameTimer dt, float tickPerSec, float duration, float distancePerDuration);
	void DrawPaths(CommandList& commandList);
	void DrawControlPoints(CommandList& commandList);
	XMVECTOR GetDirection();
	XMVECTOR GetPosition();

private:
	void GetPointStrip();
	void CalcSubPoints();
	void BuildFunctions();
	void BuildAdaptiveTable(float threshHold);
	void ArcLengthToPosition(float arcLength);
	float DistanceTimeFunction(float tick);
	float VelocityTimeFunction(float tick);
	XMVECTOR CalcA(XMVECTOR p_0, XMVECTOR p_1, XMVECTOR p_2);
	XMVECTOR CalcB(XMVECTOR p_0, XMVECTOR p_1, XMVECTOR p_2);
	int GetBezierIndex(float u);
	float DenormalizeU(float globalU, int index);
	XMFLOAT3 GetPointDistances(float u_a, float u_b, float u_m);

private:
	std::vector<XMVECTOR> mControlPoints;
	std::vector<XMVECTOR> mSubPoints;
	std::vector<std::function<XMVECTOR(float)>> mBezierEquations;
	std::vector<XMFLOAT3> mPathLines;
	std::map<float, float> mParamArcLengthMap;//key: parameter, value: arc length
	std::map<float, float> mArcLengthParamMap;//key: arc length , value: parameter
	float mWorldArcLength;

	XMVECTOR mCurrentFrameRotation;
	XMVECTOR mCurrentPosition;

	int mSlice;
	float mDeltaU;
	float mTickAccumulating;

	std::shared_ptr<Model> mControlPointModel;
};


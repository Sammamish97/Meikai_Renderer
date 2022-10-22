#pragma once
#include <vector>
#include <DirectXMath.h>
#include <functional>
#include <map>

#include "GameTimer.h"

using namespace DirectX;
class CommandList;
class PathGenerator
{
public:
	PathGenerator();
	XMVECTOR Update(GameTimer dt);
	void Draw(CommandList& commandList);
	void GetPointStrip();
	void CalcSubPoints();
	void BuildFunctions();
	void BuildAdaptiveTable(float threshHold);
	//void BuildForwardTable();
	XMVECTOR ArcLengthToPosition(float arcLength);
	float DistanceTimeFunction(float speed);

private:
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
	std::map<float, float> mArcLengthParamMap;//key: parameter, value: arc length 

	int mSlice;
	float mTimeAccumulating;
};


#pragma once
#include <vector>
#include <DirectXMath.h>
#include <functional>
#include <map>

#include "GameTimer.h"

struct EquationData
{
	EquationData() = default;
	EquationData(float arclength, float u) : ArcLength(arclength), U(u){}
	float ArcLength;
	float U;
};

struct ArcMapData
{
	ArcMapData() = default;
	ArcMapData(int index, float u) : ArrayIndex(index), U(u){}
	int ArrayIndex;
	float U;
};
using namespace DirectX;
class CommandList;
class PathGenerator
{
public:
	PathGenerator();
	XMVECTOR Update(GameTimer dt);
	void Draw(CommandList& commandList);
	void GetPointStrip();
	void ClacSubPoints();
	void BuildFunctions();
	void BuildArcTable();
	XMVECTOR ArcLengthToPosition(float arcLength);
	float DistanceTimeFunction(float speed);

private:
	XMVECTOR CalcA(XMVECTOR p_0, XMVECTOR p_1, XMVECTOR p_2);
	XMVECTOR CalcB(XMVECTOR p_0, XMVECTOR p_1, XMVECTOR p_2);

private:
	std::vector<XMVECTOR> mControlPoints;
	std::vector<XMVECTOR> mSubPoints;
	std::vector<std::function<XMVECTOR(float)>> mBezierEquations;
	std::vector<XMFLOAT3> mPathLines;

	std::vector<EquationData> mArcArray;
	std::map<float, ArcMapData> mArcMap;
	int mSlice;
	float mTimeAccumulating;
};


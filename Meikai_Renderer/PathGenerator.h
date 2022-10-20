#pragma once
#include <vector>
#include <DirectXMath.h>
#include <functional>
#include <map>

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
	int U;
};
using namespace DirectX;
class CommandList;
class PathGenerator
{
public:
	PathGenerator();
	void Update(float dt);
	void Draw(CommandList& commandList);
	void GetPointStrip();
	void ClacSubPoints();
	void BuildFunctions();
	void BuildArcTable();
	XMVECTOR GetPosition(float arcLength);
	float DistanceTimeFunction(float animTick, float speed);

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
};


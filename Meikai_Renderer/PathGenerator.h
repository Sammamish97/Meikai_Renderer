#pragma once
#include <vector>
#include <DirectXMath.h>
using namespace DirectX;
class CommandList;
class PathGenerator
{
public:
	PathGenerator();
	void Update(float dt);
	void Draw(CommandList& commandList);
	XMFLOAT3 CalcCubicBezier(float t);

private:
	std::vector<XMFLOAT3> controlPoints;
	std::vector<XMFLOAT3> pathLines;

};


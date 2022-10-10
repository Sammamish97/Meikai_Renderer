#include "PathGenerator.h"
PathGenerator::PathGenerator()
{
	//Use X, Z Plane. Y value will always 0.
	controlPoints.push_back(XMFLOAT3(5, 0, 5));
	controlPoints.push_back(XMFLOAT3(-5, 0, 5));
	controlPoints.push_back(XMFLOAT3(-5, 0, -5));
	controlPoints.push_back(XMFLOAT3(5, 0, -5));

}

XMFLOAT3 PathGenerator::CalcCubicBezier(float t)
{
	return XMFLOAT3(0, 0, 0);
}

void PathGenerator::Update(float dt)
{

}

void PathGenerator::Draw(CommandList& commandList)
{

}
#pragma once
#include <intsafe.h>

struct LightPassIndices
{
	const UINT TextureNum = 6;
	UINT Pos;
	UINT Normal;
	UINT Albedo;
	UINT Roughness;
	UINT Metalic;
	UINT SSAO;
};

struct SkyboxPassIndices
{
	const UINT TextureNum = 1;

	UINT Skybox;
};


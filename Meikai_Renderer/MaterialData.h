#pragma once
#include <DirectXMath.h>

struct MaterialData
{
	MaterialData(DirectX::XMFLOAT3 albedo, float metalic, float roughness)
		:AlbedoAndBool(albedo.x, albedo.y, albedo.z,1), Metalic(metalic), Roughness(roughness) {}
	DirectX::XMFLOAT4 AlbedoAndBool;
	float Metalic;
	float Roughness;
};

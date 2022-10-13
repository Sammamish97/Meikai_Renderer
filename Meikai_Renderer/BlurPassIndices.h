#pragma once
#include <intsafe.h>

struct BlurPassIndices
{
	const UINT TexNum = 4;
	UINT Normal;
	UINT Depth;
	UINT SRVTextureInput;
	UINT UAVTextureOutput;
};

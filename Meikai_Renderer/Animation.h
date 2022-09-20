#pragma once
#include <vector>
#include <string>

struct KeyFrameData
{
	
};

class Animation
{
	void LoadAnimation(std::string file_path);

	std::vector<KeyFrameData> mKeyFrameDatas;
};


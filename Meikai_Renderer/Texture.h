#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <string>

using Microsoft::WRL::ComPtr;

class DXApp;
struct Texture
{
	Texture(DXApp* app, ComPtr<ID3D12GraphicsCommandList2> commandList, std::string imagePath);
	
	//Staging resource must not deleted before staging command is flushed by queue.
	//Therefore, before command list manager implemented, need to cache staging resource.
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> staging;
	
	DXGI_FORMAT format;
	std::string path;

	int width;
	int height;
	int colorChannels;
};


#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <string>

using Microsoft::WRL::ComPtr;

class DXApp;
struct Texture
{
	//Staging resource must not deleted before staging command is flushed by queue.
	//Therefore, before command list manager implemented, need to cache staging resource.
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> staging;
	
	DXGI_FORMAT format;
	std::wstring path;
};


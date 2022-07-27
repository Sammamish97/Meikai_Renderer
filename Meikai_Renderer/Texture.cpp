#include "Texture.h"
#include "DXApp.h"
#include "DXUtil.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(DXApp* app, ComPtr<ID3D12GraphicsCommandList2> commandList, std::string imagePath)
	:path(imagePath)
{
	unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &colorChannels, 0);
	app->UpdateDefaultBufferResource(commandList, resource.GetAddressOf(), staging.GetAddressOf(), width * height, sizeof(unsigned char), data);
	stbi_image_free(data);
}
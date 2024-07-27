#include "Texture.h"

namespace Engine
{
namespace Graphics
{
	Texture::Texture(ID3D11Texture2D* _texture)
	{
		texture = _texture;
	}
	Texture::~Texture()
	{
		if (texture != nullptr)
			texture->Release();
	}
}
}
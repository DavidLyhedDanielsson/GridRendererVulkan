#pragma once

#include "ResourceManager.h"

enum class TexelComponentCount
{
	SINGLE,
	QUAD
};

enum class TexelComponentSize
{
	BYTE,
	WORD
};

enum class TexelComponentType
{
	FLOAT,
	UNORM,
	DEPTH,
};

struct FormatInfo
{
	TexelComponentCount componentCount = TexelComponentCount::QUAD;
	TexelComponentSize componentSize = TexelComponentSize::BYTE;
	TexelComponentType componentType = TexelComponentType::UNORM;
};

enum TextureBinding
{
	NONE = 0,
	SHADER_RESOURCE = 1,
	UNORDERED_ACCESS = 2,
	RENDER_TARGET = 4,
	DEPTH_STENCIL = 8
};

struct TextureInfo
{
	unsigned int mipLevels = 1;
	unsigned int baseTextureWidth = 0;
	unsigned int baseTextureHeight = 0;
	FormatInfo format;
	unsigned int bindingFlags = TextureBinding::NONE;
};

class TextureManager : public ResourceManager
{
protected:

public:
	TextureManager() = default;
	virtual ~TextureManager() = default;
	TextureManager(const TextureManager& other) = delete;
	TextureManager& operator=(const TextureManager& other) = delete;
	TextureManager(TextureManager&& other) = default;
	TextureManager& operator=(TextureManager&& other) = default;

	//Only textures with power of 2 base width/height need to be supported
	virtual ResourceIndex AddTexture(void* textureData,
		const TextureInfo& textureInfo) = 0;
};
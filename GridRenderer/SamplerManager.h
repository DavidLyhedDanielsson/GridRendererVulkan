#pragma once

#include "ResourceManager.h"

enum class SamplerType
{
	POINT,
	BILINEAR,
	ANISOTROPIC
};

enum class AddressMode
{
	WRAP,
	CLAMP,
	BLACK_BORDER_COLOUR
};

class SamplerManager : public ResourceManager
{
protected:

public:
	SamplerManager() = default;
	virtual ~SamplerManager() = default;
	SamplerManager(const SamplerManager& other) = delete;
	SamplerManager& operator=(const SamplerManager& other) = delete;
	SamplerManager(SamplerManager&& other) = default;
	SamplerManager& operator=(SamplerManager&& other) = default;

	virtual ResourceIndex CreateSampler(SamplerType type, AddressMode adressMode) = 0;
};
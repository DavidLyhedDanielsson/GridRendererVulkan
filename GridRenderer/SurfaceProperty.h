#pragma once

#include "ResourceManager.h"

class SurfaceProperty
{
protected:
	ResourceIndex diffuseTexture = ResourceIndex(-1);
	ResourceIndex specularTexture = ResourceIndex(-1);
	ResourceIndex sampler = ResourceIndex(-1);

public:
	SurfaceProperty() = default;
	virtual ~SurfaceProperty() = default;
	SurfaceProperty(const SurfaceProperty& other) = default;
	SurfaceProperty& operator=(const SurfaceProperty& other) = default;
	SurfaceProperty(SurfaceProperty&& other) = default;
	SurfaceProperty& operator=(SurfaceProperty&& other) = default;

	void SetDiffuseTexture(ResourceIndex index = 0);
	void SetSpecularTexture(ResourceIndex index = 0);

	ResourceIndex GetDiffuseTexture() const;
	ResourceIndex GetSpecularTexture() const;
	ResourceIndex GetSampler() const;
};
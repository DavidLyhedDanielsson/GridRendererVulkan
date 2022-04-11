#include "SurfaceProperty.h"

void SurfaceProperty::SetDiffuseTexture(ResourceIndex index)
{
	diffuseTexture = index;
}

void SurfaceProperty::SetSpecularTexture(ResourceIndex index)
{
	specularTexture = index;
}

ResourceIndex SurfaceProperty::GetDiffuseTexture() const
{
	return diffuseTexture;
}

ResourceIndex SurfaceProperty::GetSpecularTexture() const
{
	return specularTexture;
}

ResourceIndex SurfaceProperty::GetSampler() const
{
	return sampler;
}

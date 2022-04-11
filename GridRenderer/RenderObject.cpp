#include "RenderObject.h"

void RenderObject::Initialise(ResourceIndex objectTransformBuffer, 
	const Mesh& objectMesh, const SurfaceProperty& objectSurfaceProperty)
{
	transformBuffer = objectTransformBuffer;
	mesh = objectMesh;
	surfaceProperty = objectSurfaceProperty;
}

ResourceIndex RenderObject::GetTransformBufferIndex() const
{
	return transformBuffer;
}

const Mesh& RenderObject::GetMesh() const
{
	return mesh;
}

const SurfaceProperty& RenderObject::GetSurfaceProperty() const
{
	return surfaceProperty;
}

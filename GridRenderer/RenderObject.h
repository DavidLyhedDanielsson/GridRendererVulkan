#pragma once

#include "BufferManager.h"
#include "Mesh.h"
#include "SurfaceProperty.h"

class RenderObject
{
private:
	ResourceIndex transformBuffer;
	Mesh mesh;
	SurfaceProperty surfaceProperty;

public:
	RenderObject() = default;
	virtual ~RenderObject() = default;
	RenderObject(const RenderObject& other) = default;
	RenderObject& operator=(const RenderObject& other) = default;
	RenderObject(RenderObject&& other) = default;
	RenderObject& operator=(RenderObject&& other) = default;

	void Initialise(ResourceIndex objectTransformBuffer,
		const Mesh& objectMesh, const SurfaceProperty& objectSurfaceProperty);

	ResourceIndex GetTransformBufferIndex() const;
	const Mesh& GetMesh() const;
	const SurfaceProperty& GetSurfaceProperty() const;
};
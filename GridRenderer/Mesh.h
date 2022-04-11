#pragma once

#include "ResourceManager.h"

class Mesh
{
protected:
	ResourceIndex vertexBuffer = ResourceIndex(-1);
	ResourceIndex indexBuffer = ResourceIndex(-1);

public:
	Mesh() = default;
	virtual ~Mesh() = default;
	Mesh(const Mesh& other) = default;
	Mesh& operator=(const Mesh& other) = default;
	Mesh(Mesh&& other) = default;
	Mesh& operator=(Mesh&& other) = default;

	void SetVertexBuffer(ResourceIndex index);
	void SetIndexBuffer(ResourceIndex index);
	ResourceIndex GetVertexBuffer() const;
	ResourceIndex GetIndexBuffer() const;
};
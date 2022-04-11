#include "Mesh.h"

void Mesh::SetVertexBuffer(ResourceIndex index)
{
	vertexBuffer = index;
}

void Mesh::SetIndexBuffer(ResourceIndex index)
{
	indexBuffer = index;
}

ResourceIndex Mesh::GetVertexBuffer() const
{
	return vertexBuffer;
}

ResourceIndex Mesh::GetIndexBuffer() const
{
	return indexBuffer;
}

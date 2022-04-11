#pragma once

#include "ResourceManager.h"

enum class PerFrameWritePattern
{
	NEVER,
	ONCE,
	MULTIPLE
};

enum BufferBinding
{
	STRUCTURED_BUFFER = 1,
	CONSTANT_BUFFER = 2
};

class BufferManager : public ResourceManager
{
protected:
	
public:
	BufferManager() = default;
	virtual ~BufferManager() = default;
	BufferManager(const BufferManager& other) = delete;
	BufferManager& operator=(const BufferManager& other) = delete;
	BufferManager(BufferManager&& other) = default;
	BufferManager& operator=(BufferManager&& other) = default;

	virtual ResourceIndex AddBuffer(void* data, unsigned int elementSize,
		unsigned int nrOfElements, PerFrameWritePattern cpuWrite, 
		PerFrameWritePattern gpuWrite, unsigned int bindingFlags) = 0;

	virtual void UpdateBuffer(ResourceIndex index, void* data) = 0;
	virtual unsigned int GetElementSize(ResourceIndex index) = 0;
	virtual unsigned int GetElementCount(ResourceIndex index) = 0;
};
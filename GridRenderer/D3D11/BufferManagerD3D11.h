#pragma once

#include <vector>

#include <d3d11_4.h>

#include "../BufferManager.h"

class BufferManagerD3D11 : public BufferManager
{
private:
	struct StoredBuffer
	{
		ID3D11Buffer* interfacePtr = nullptr;
		unsigned int elementSize = 0;
		unsigned int elementCount = 0;
		ID3D11ShaderResourceView* srv;
	};

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	std::vector<StoredBuffer> buffers;

	bool DetermineUsage(PerFrameWritePattern cpuWrite,
		PerFrameWritePattern gpuWrite, D3D11_USAGE& usage);
	UINT TranslateBindFlags(unsigned int bindingFlags);
	bool CreateDescription(unsigned int elementSize,
		unsigned int nrOfElements, PerFrameWritePattern cpuWrite,
		PerFrameWritePattern gpuWrite, unsigned int bindingFlags,
		D3D11_BUFFER_DESC& toSet);

	ID3D11ShaderResourceView* CreateSRV(ID3D11Buffer* buffer, unsigned int stride);

public:
	BufferManagerD3D11() = default;
	virtual ~BufferManagerD3D11();
	BufferManagerD3D11(const BufferManagerD3D11& other) = delete;
	BufferManagerD3D11& operator=(const BufferManagerD3D11& other) = delete;
	BufferManagerD3D11(BufferManagerD3D11&& other) = default;
	BufferManagerD3D11& operator=(BufferManagerD3D11&& other) = default;

	void Initialise(ID3D11Device* deviceToUse,
		ID3D11DeviceContext* contextToUse);

	ResourceIndex AddBuffer(void* data, unsigned int elementSize,
		unsigned int nrOfElements, PerFrameWritePattern cpuWrite,
		PerFrameWritePattern gpuWrite, unsigned int bindingFlags) override;

	void UpdateBuffer(ResourceIndex index, void* data) override;
	unsigned int GetElementSize(ResourceIndex index) override;
	unsigned int GetElementCount(ResourceIndex index) override;

	ID3D11Buffer* GetBufferInterface(ResourceIndex index);
	ID3D11ShaderResourceView* GetSRV(ResourceIndex index);
};
#pragma once

#include <vector>

#include <d3d11_4.h>

#include "../SamplerManager.h"

class SamplerManagerD3D11 : public SamplerManager
{
private:
	ID3D11Device* device = nullptr;
	std::vector<ID3D11SamplerState*> samplers;

	void SetFilter(D3D11_SAMPLER_DESC& toSetIn,
		SamplerType type);
	void SetAdressMode(D3D11_SAMPLER_DESC& toSetIn,
		AddressMode adressMode);

public:
	SamplerManagerD3D11() = default;
	virtual ~SamplerManagerD3D11();
	SamplerManagerD3D11(const SamplerManagerD3D11& other) = delete;
	SamplerManagerD3D11& operator=(const SamplerManagerD3D11& other) = delete;
	SamplerManagerD3D11(SamplerManagerD3D11&& other) = default;
	SamplerManagerD3D11& operator=(SamplerManagerD3D11&& other) = default;

	void Initialise(ID3D11Device* deviceToUse);

	ResourceIndex CreateSampler(SamplerType type, AddressMode adressMode) override;

	ID3D11SamplerState* GetSampler(ResourceIndex index);
};
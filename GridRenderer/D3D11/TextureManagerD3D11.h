#pragma once

#include <d3d11_4.h>
#include <vector>

#include "../TextureManager.h"

class TextureManagerD3D11 : public TextureManager
{
private:
	struct TextureViews
	{
		ID3D11ShaderResourceView* srv = nullptr;
		ID3D11UnorderedAccessView* uav = nullptr;
		ID3D11RenderTargetView* rtv = nullptr;
		ID3D11DepthStencilView* dsv = nullptr;
	};

	struct StoredTexture
	{
		ID3D11Texture2D* interfacePtr = nullptr;
		TextureViews views;
	};

	ID3D11Device* device = nullptr;
	std::vector<StoredTexture> textures;

	bool TranslateFormatInfo(const FormatInfo& formatInfo,
		DXGI_FORMAT& toSet);
	D3D11_USAGE DetermineUsage(unsigned int bindingFlags);
	UINT TranslateBindFlags(unsigned int bindingFlags);
	bool CreateDescription(const TextureInfo& textureInfo,
		D3D11_TEXTURE2D_DESC& toSet);
	bool CreateResourceViews(ID3D11Texture2D* texture, unsigned int bindingFlags,
		TextureViews& toSet);

public:
	TextureManagerD3D11() = default;
	virtual ~TextureManagerD3D11();
	TextureManagerD3D11(const TextureManagerD3D11& other) = delete;
	TextureManagerD3D11& operator=(const TextureManagerD3D11& other) = delete;
	TextureManagerD3D11(TextureManagerD3D11&& other) = default;
	TextureManagerD3D11& operator=(TextureManagerD3D11&& other) = default;

	void Initialise(ID3D11Device* deviceToUse);

	ResourceIndex AddTexture(void* textureData, 
		const TextureInfo& textureInfo) override;

	ID3D11ShaderResourceView* GetSRV(ResourceIndex index);
};
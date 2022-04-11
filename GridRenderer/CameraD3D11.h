#pragma once

#include <DirectXMath.h>
#include <d3d11_4.h>

#include "Camera.h"
#include "BufferManagerD3D11.h"

class CameraD3D11 : public Camera
{
private:
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 right;

	DirectX::XMFLOAT4X4 projectionMatrix;

	ResourceIndex vpBufferIndex = ResourceIndex(-1);
	ResourceIndex cameraPosBufferIndex = ResourceIndex(-1);

	void CreateProjectionMatrix(float minDepth, float maxDepth, float aspectRatio);

public:
	CameraD3D11(BufferManagerD3D11& bufferManager, float minDepth,
		float maxDepth, float aspectRatio);
	virtual ~CameraD3D11() = default;
	CameraD3D11(const CameraD3D11& other) = default;
	CameraD3D11& operator=(const CameraD3D11& other) = default;
	CameraD3D11(CameraD3D11&& other) = default;
	CameraD3D11& operator=(CameraD3D11&& other) = default;

	void MoveForward(float amount) override;
	void MoveUp(float amount) override;
	void MoveRight(float amount) override;
	void RotateY(float radians) override;

	ResourceIndex GetVP(BufferManagerD3D11& bufferManager);
	ResourceIndex GetPosition(BufferManagerD3D11& bufferManager);
};
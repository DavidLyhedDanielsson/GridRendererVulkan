#include "CameraD3D11.h"

using namespace DirectX;

void CameraD3D11::CreateProjectionMatrix(float minDepth,
	float maxDepth, float aspectRatio)
{
	XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2,
		aspectRatio, minDepth, maxDepth);
	XMStoreFloat4x4(&projectionMatrix, projection);
}

CameraD3D11::CameraD3D11(BufferManagerD3D11& bufferManager, float minDepth,
	float maxDepth, float aspectRatio)
{
	CreateProjectionMatrix(minDepth, 
		maxDepth, aspectRatio);
	position = { 0.0f, 0.0f, -4.0f };
	forward = { 0.0f, 0.0f, 1.0f };
	up = { 0.0f, 1.0f, 0.0f };
	right = { 1.0f, 0.0f, 0.0f };

	vpBufferIndex = bufferManager.AddBuffer(nullptr, sizeof(XMFLOAT4X4),
		1, PerFrameWritePattern::ONCE, PerFrameWritePattern::NEVER,
		BufferBinding::CONSTANT_BUFFER);
	cameraPosBufferIndex = bufferManager.AddBuffer(nullptr, 
		sizeof(XMFLOAT3), 1, PerFrameWritePattern::ONCE, 
		PerFrameWritePattern::NEVER, BufferBinding::CONSTANT_BUFFER);
}

void CameraD3D11::MoveForward(float amount)
{
	XMVECTOR currentPos = XMLoadFloat3(&position);
	XMVECTOR forwardVector = XMLoadFloat3(&forward);
	currentPos += forwardVector * amount;
	XMStoreFloat3(&position, currentPos);
}

void CameraD3D11::MoveUp(float amount)
{
	XMVECTOR currentPos = XMLoadFloat3(&position);
	XMVECTOR upVector = XMLoadFloat3(&up);
	currentPos += upVector * amount;
	XMStoreFloat3(&position, currentPos);
}

void CameraD3D11::MoveRight(float amount)
{
	XMVECTOR currentPos = XMLoadFloat3(&position);
	XMVECTOR rightVector = XMLoadFloat3(&right);
	currentPos += rightVector * amount;
	XMStoreFloat3(&position, currentPos);
}

void CameraD3D11::RotateY(float radians)
{
	XMVECTOR axis = XMLoadFloat3(&up);
	XMMATRIX rotationMatrix = XMMatrixRotationAxis(axis, radians);
	XMVECTOR forwardVector = XMLoadFloat3(&forward);
	XMVECTOR rightVector = XMLoadFloat3(&right);
	XMStoreFloat3(&forward, XMVector3Transform(forwardVector, rotationMatrix));
	XMStoreFloat3(&right, XMVector3Transform(rightVector, rotationMatrix));
}

ResourceIndex CameraD3D11::GetVP(BufferManagerD3D11& bufferManager)
{
	XMVECTOR eyePos = XMLoadFloat3(&position);
	XMVECTOR forwardVector = XMLoadFloat3(&forward);
	XMVECTOR upVector = XMLoadFloat3(&up);

	XMMATRIX view = XMMatrixLookAtLH(eyePos,
		XMVectorAdd(eyePos, forwardVector), upVector);
	XMMATRIX projection = XMLoadFloat4x4(&projectionMatrix);

	XMFLOAT4X4 toUpload;
	XMStoreFloat4x4(&toUpload, XMMatrixTranspose(view * projection));

	bufferManager.UpdateBuffer(vpBufferIndex, &toUpload);

	return vpBufferIndex;
}

ResourceIndex CameraD3D11::GetPosition(BufferManagerD3D11& bufferManager)
{
	bufferManager.UpdateBuffer(cameraPosBufferIndex, &position);
	return cameraPosBufferIndex;
}

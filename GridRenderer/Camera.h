#pragma once

class Camera
{
protected:

public:
	Camera() = default;
	virtual ~Camera() = default;
	Camera(const Camera& other) = default;
	Camera& operator=(const Camera& other) = default;
	Camera(Camera&& other) = default;
	Camera& operator=(Camera&& other) = default;

	virtual void MoveForward(float amount) = 0;
	virtual void MoveUp(float amount) = 0;
	virtual void MoveRight(float amount) = 0;
	virtual void RotateY(float radians) = 0;
};
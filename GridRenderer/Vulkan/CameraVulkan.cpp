#include "CameraVulkan.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "BufferManagerVulkan.h"

CameraVulkan::CameraVulkan(
    BufferManagerVulkan& bufferManager,
    float minDepth,
    float maxDepth,
    float aspectRatio)
{
    projectionMatrix = glm::perspective(glm::radians(90.0f), aspectRatio, minDepth, maxDepth);
    projectionMatrix[1][1] *= -1.0f;

    position = {0.0f, 0.0f, -4.0f};
    forward = {0.0f, 0.0f, 1.0f};
    up = {0.0f, 1.0f, 0.0f};
    right = {1.0f, 0.0f, 0.0f};
}

void CameraVulkan::MoveForward(float amount)
{
    position += forward * amount;
}

void CameraVulkan::MoveUp(float amount)
{
    position += up * amount;
}

void CameraVulkan::MoveRight(float amount)
{
    position += right * amount;
}

void CameraVulkan::RotateY(float radians)
{
    // TODO
}
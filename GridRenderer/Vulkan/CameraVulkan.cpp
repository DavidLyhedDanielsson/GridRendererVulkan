#include "CameraVulkan.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#if !GLM_FORCE_DEPTH_ZERO_TO_ONE || !GLM_FORCE_RADIANS
    #error GLM compile flags are not set
#endif

CameraVulkan::CameraVulkan(
    BufferManagerVulkan& bufferManager,
    float minDepth,
    float maxDepth,
    float aspectRatio)
{
    this->projectionMatrix = glm::perspective(glm::radians(90.0f), aspectRatio, minDepth, maxDepth);
    this->projectionMatrix[1][1] *= -1.0f;

    this->position = {0.0f, 0.0f, -4.0f};
    this->forward = {0.0f, 0.0f, -1.0f};
    this->up = {0.0f, 1.0f, 0.0f};
    this->right = {1.0f, 0.0f, 0.0f};

    this->cameraPositionBufferIndex = bufferManager.AddBuffer(
        &position,
        sizeof(float),
        4,
        PerFrameWritePattern::NEVER,
        PerFrameWritePattern::NEVER,
        BufferBinding::CONSTANT_BUFFER);
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
    forward = glm::rotate(forward, -radians, up);
    right = glm::rotate(right, -radians, up);
}

glm::vec3 CameraVulkan::GetPosition() const
{
    return position;
}

glm::mat4 CameraVulkan::GetViewProjMatrix() const
{
    return projectionMatrix * glm::lookAt(position, position + forward, up);
}

ResourceIndex CameraVulkan::GetCameraPositionBufferIndex() const
{
    return cameraPositionBufferIndex;
}
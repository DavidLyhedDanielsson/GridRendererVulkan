#include "CameraVulkan.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "BufferManagerVulkan.h"

#if !GLM_FORCE_DEPTH_ZERO_TO_ONE || !GLM_FORCE_RADIANS
    #error GLM compile flags are not set
#endif

CameraVulkan::CameraVulkan(
    BufferManagerVulkan& bufferManager,
    float minDepth,
    float maxDepth,
    float aspectRatio)
{
    projectionMatrix = glm::perspective(glm::radians(90.0f), aspectRatio, minDepth, maxDepth);
    projectionMatrix[1][1] *= -1.0f;

    position = {0.0f, 0.0f, -4.0f};
    forward = {0.0f, 0.0f, -1.0f};
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
    forward = glm::rotate(forward, -radians, up);
    right = glm::rotate(right, -radians, up);
}

glm::mat4 CameraVulkan::getViewProjMatrix()
{
    return projectionMatrix * glm::lookAt(position, position + forward, up);
}
#pragma once

#include <glm/matrix.hpp>

#include "../Camera.h"

// Forward-declaration reduces compile times
class BufferManagerVulkan;

class CameraVulkan: public Camera
{
  private:
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;

    glm::mat4 projectionMatrix;

  public:
    CameraVulkan(
        BufferManagerVulkan& bufferManager,
        float minDepth,
        float maxDepth,
        float aspectRatio);
    CameraVulkan(const CameraVulkan& other) = default;
    CameraVulkan& operator=(const CameraVulkan& other) = default;
    CameraVulkan(CameraVulkan&& other) = default;
    CameraVulkan& operator=(CameraVulkan&& other) = default;

    void MoveForward(float amount) override;
    void MoveUp(float amount) override;
    void MoveRight(float amount) override;
    void RotateY(float radians) override;

    glm::mat4 getViewProjMatrix();
};
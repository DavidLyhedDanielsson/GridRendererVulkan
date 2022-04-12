#include "RendererVulkan.h"

#include <SDL2/SDL_video.h>

RendererVulkan::RendererVulkan(SDL_Window *windowHandle) {

}

GraphicsRenderPass *RendererVulkan::CreateGraphicsRenderPass(const GraphicsRenderPassInfo &intialisationInfo) {
    return nullptr;
}

void RendererVulkan::DestroyGraphicsRenderPass(GraphicsRenderPass *pass) {

}

Camera *RendererVulkan::CreateCamera(float minDepth, float maxDepth, float aspectRatio) {
    return nullptr;
}

void RendererVulkan::DestroyCamera(Camera *camera) {

}

BufferManager *RendererVulkan::GetBufferManager() {
    return nullptr;
}

TextureManager *RendererVulkan::GetTextureManager() {
    return nullptr;
}

SamplerManager *RendererVulkan::GetSamplerManager() {
    return nullptr;
}

void RendererVulkan::SetRenderPass(GraphicsRenderPass *toSet) {

}

void RendererVulkan::SetCamera(Camera *toSet) {

}

void RendererVulkan::SetLightBuffer(ResourceIndex lightBufferIndexToUse) {

}

void RendererVulkan::PreRender() {

}

void RendererVulkan::Render(const std::vector<RenderObject> &objectsToRender) {

}

void RendererVulkan::Present() {

}

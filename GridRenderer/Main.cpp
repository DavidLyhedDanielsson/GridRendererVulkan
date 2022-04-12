#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <SDL2/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef USE_VULKAN
#include "Vulkan/RendererVulkan.h"
#elif USE_D3D11
#include "D3D11/RendererD3D11.h"
#endif

struct Inputs
{
    bool moveLeftPushed = false;
    bool moveRightPushed = false;
    bool moveForwardPushed = false;
    bool moveBackwardsPushed = false;
    bool moveUpPushed = false;
    bool moveDownPushed = false;

    bool turnLeftPushed = false;
    bool turnRightPushed = false;

    bool quitKey = false;
} globalInputs;

struct SimpleVertex
{
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float uv[2] = { 0.0f, 0.0f };
    float normal[3] = { 0.0f, 0.0f, 0.0f };
};

struct PointLight
{
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float colour[3] = { 1.0f, 1.0f, 1.0f };
};

void HandleKeyEvent(SDL_Keysym sym, bool value)
{
    switch (sym.sym)
    {
    case SDLK_a:
        globalInputs.moveLeftPushed = value;
        break;
    case SDLK_d:
        globalInputs.moveRightPushed = value;
        break;
    case SDLK_w:
        globalInputs.moveForwardPushed = value;
        break;
    case SDLK_s:
        globalInputs.moveBackwardsPushed = value;
        break;
    case SDLK_LSHIFT:
        globalInputs.moveUpPushed = value;
        break;
    case SDLK_LCTRL:
        globalInputs.moveDownPushed = value;
        break;
    case SDLK_ESCAPE:
        globalInputs.quitKey = value;
        break;
    case SDLK_q:
        globalInputs.turnLeftPushed = value;
        break;
    case SDLK_e:
        globalInputs.turnRightPushed = value;
        break;
    }
}

SDL_Window* InitialiseWindow(unsigned int windowWidth,
    unsigned int windowHeight, bool windowed = true)
{
    int windowFlags = SDL_WINDOW_SHOWN;
    if (!windowed) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Grid Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, windowFlags);

    return window;
}

GraphicsRenderPass* CreateStandardRenderPass(Renderer* renderer)
{
    GraphicsRenderPassInfo info;
    info.vsPath = SHADER_ROOT_DIR "shaders/StandardVS.cso";
    info.psPath = SHADER_ROOT_DIR "shaders/StandardPS.cso";

    PipelineBinding vertexBinding;
    vertexBinding.dataType = PipelineDataType::VERTEX;
    vertexBinding.bindingType = PipelineBindingType::SHADER_RESOURCE;
    vertexBinding.shaderStage = PipelineShaderStage::VS;
    vertexBinding.slotToBindTo = 0;
    info.objectBindings.push_back(vertexBinding);

    PipelineBinding indicesBinding;
    indicesBinding.dataType = PipelineDataType::INDEX;
    indicesBinding.bindingType = PipelineBindingType::SHADER_RESOURCE;
    indicesBinding.shaderStage = PipelineShaderStage::VS;
    indicesBinding.slotToBindTo = 1;
    info.objectBindings.push_back(indicesBinding);

    PipelineBinding transformBinding;
    transformBinding.dataType = PipelineDataType::TRANSFORM;
    transformBinding.bindingType = PipelineBindingType::CONSTANT_BUFFER;
    transformBinding.shaderStage = PipelineShaderStage::VS;
    transformBinding.slotToBindTo = 0;
    info.objectBindings.push_back(transformBinding);

    PipelineBinding vpBinding;
    vpBinding.dataType = PipelineDataType::VIEW_PROJECTION;
    vpBinding.bindingType = PipelineBindingType::CONSTANT_BUFFER;
    vpBinding.shaderStage = PipelineShaderStage::VS;
    vpBinding.slotToBindTo = 1;
    info.globalBindings.push_back(vpBinding);

    PipelineBinding diffuseTextureBinding;
    diffuseTextureBinding.dataType = PipelineDataType::DIFFUSE;
    diffuseTextureBinding.bindingType = PipelineBindingType::SHADER_RESOURCE;
    diffuseTextureBinding.shaderStage = PipelineShaderStage::PS;
    diffuseTextureBinding.slotToBindTo = 0;
    info.objectBindings.push_back(diffuseTextureBinding);

    PipelineBinding specularTextureBinding;
    specularTextureBinding.dataType = PipelineDataType::SPECULAR;
    specularTextureBinding.bindingType = PipelineBindingType::SHADER_RESOURCE;
    specularTextureBinding.shaderStage = PipelineShaderStage::PS;
    specularTextureBinding.slotToBindTo = 1;
    info.objectBindings.push_back(specularTextureBinding);

    PipelineBinding lightBufferBinding;
    lightBufferBinding.dataType = PipelineDataType::LIGHT;
    lightBufferBinding.bindingType = PipelineBindingType::SHADER_RESOURCE;
    lightBufferBinding.shaderStage = PipelineShaderStage::PS;
    lightBufferBinding.slotToBindTo = 2;
    info.globalBindings.push_back(lightBufferBinding);

    PipelineBinding cameraPosBinding;
    cameraPosBinding.dataType = PipelineDataType::CAMERA_POS;
    cameraPosBinding.bindingType = PipelineBindingType::CONSTANT_BUFFER;
    cameraPosBinding.shaderStage = PipelineShaderStage::PS;
    cameraPosBinding.slotToBindTo = 0;
    info.globalBindings.push_back(cameraPosBinding);

    ResourceIndex samplerIndex = renderer->GetSamplerManager()->CreateSampler(
        SamplerType::ANISOTROPIC, AddressMode::CLAMP);

    if (samplerIndex == ResourceIndex(-1))
        throw std::runtime_error("Could not create clamp sampler");

    PipelineBinding clampSamplerBinding;
    clampSamplerBinding.dataType = PipelineDataType::SAMPLER;
    clampSamplerBinding.bindingType = PipelineBindingType::NONE;
    clampSamplerBinding.shaderStage = PipelineShaderStage::PS;
    clampSamplerBinding.slotToBindTo = 0;
    info.globalBindings.push_back(clampSamplerBinding);

    GraphicsRenderPass* toReturn = renderer->CreateGraphicsRenderPass(info);
    toReturn->SetGlobalSampler(PipelineShaderStage::PS, 0, samplerIndex);

    return toReturn;
}

bool CreateTriangleMesh(Mesh& mesh, Renderer* renderer)
{
    SimpleVertex vertices[] =
    {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
    };

    ResourceIndex verticesIndex = renderer->GetBufferManager()->AddBuffer(
        vertices, sizeof(SimpleVertex), std::size(vertices),
        PerFrameWritePattern::NEVER, PerFrameWritePattern::NEVER,
        BufferBinding::STRUCTURED_BUFFER);

    if (verticesIndex == ResourceIndex(-1))
        return false;

    unsigned int indices[] = {0, 1, 2};

    ResourceIndex indicesIndex = renderer->GetBufferManager()->AddBuffer(
        indices, sizeof(unsigned int), std::size(indices),
        PerFrameWritePattern::NEVER, PerFrameWritePattern::NEVER,
        BufferBinding::STRUCTURED_BUFFER);

    if (indicesIndex == ResourceIndex(-1))
        return false;

    mesh.SetVertexBuffer(verticesIndex);
    mesh.SetIndexBuffer(indicesIndex);

    return true;
}

bool CreateCubeMesh(Mesh& mesh, Renderer* renderer)
{
    // Order per face is top left, top right, bottom left, bottom right
    SimpleVertex vertices[] =
    {
        {{-0.5f, 0.5f, 0.5f}, {1.0f / 3, 0.0f}, {0.0f, 1.0f, 0.0f}}, // top
        {{0.5f, 0.5f, 0.5f}, {2.0f / 3, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f / 3, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {2.0f / 3, 0.5f}, {0.0f, 1.0f, 0.0f}},

        {{0.5f, 0.5f, -0.5f}, {2.0f / 3, 0.0f}, {1.0f, 0.0f, 0.0f}}, // right
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {2.0f / 3, 0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},

        {{-0.5f, 0.5f, -0.5f}, {1.0f / 3, 0.5f}, {0.0f, 0.0f, -1.0f}}, // back
        {{0.5f, 0.5f, -0.5f}, {2.0f / 3, 0.5f}, {0.0f, 0.0f, -1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f / 3, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.5f, -0.5f, -0.5f}, {2.0f / 3, 1.0f}, {0.0f, 0.0f, -1.0f}},

        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, // left
        {{-0.5f, 0.5f, -0.5f}, {1.0f / 3, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.5f}, {-1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f / 3, 0.5f}, {-1.0f, 0.0f, 0.0f}},

        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // front
        {{-0.5f, 0.5f, 0.5f}, {1.0f / 3, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f / 3, 1.0f}, {0.0f, 0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {2.0f / 3, 0.5f}, {0.0f, 0.0f, -1.0f}}, // bottom
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.5f}, {0.0f, 0.0f, -1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {2.0f / 3, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}
    };

    ResourceIndex verticesIndex = renderer->GetBufferManager()->AddBuffer(
        vertices, sizeof(SimpleVertex), std::size(vertices),
        PerFrameWritePattern::NEVER, PerFrameWritePattern::NEVER,
        BufferBinding::STRUCTURED_BUFFER);

    if (verticesIndex == ResourceIndex(-1))
        return false;

    const unsigned int NR_OF_INDICES = 36;
    unsigned int indices[NR_OF_INDICES];
    for (unsigned int i = 0; i < NR_OF_INDICES / 6; ++i)
    {
        unsigned int baseBufferIndex = i * 6;
        unsigned int baseVertexIndex = i * 4;
        indices[baseBufferIndex + 0] = baseVertexIndex + 0;
        indices[baseBufferIndex + 1] = baseVertexIndex + 1;
        indices[baseBufferIndex + 2] = baseVertexIndex + 2;
        indices[baseBufferIndex + 3] = baseVertexIndex + 2;
        indices[baseBufferIndex + 4] = baseVertexIndex + 1;
        indices[baseBufferIndex + 5] = baseVertexIndex + 3;
    }

    ResourceIndex indicesIndex = renderer->GetBufferManager()->AddBuffer(
        indices, sizeof(unsigned int), NR_OF_INDICES, PerFrameWritePattern::NEVER,
        PerFrameWritePattern::NEVER, BufferBinding::STRUCTURED_BUFFER);

    if (indicesIndex == ResourceIndex(-1))
        return false;

    mesh.SetVertexBuffer(verticesIndex);
    mesh.SetIndexBuffer(indicesIndex);

    return true;
}

bool LoadTexture(ResourceIndex& toSet,
    Renderer* renderer, std::string filePath, unsigned int components)
{
    filePath = CONTENT_ROOT_DIR + filePath;
    int width, height;
    unsigned char* imageData = stbi_load(filePath.c_str(),
        &width, &height, nullptr, components);

    TextureInfo textureInfo;
    textureInfo.baseTextureWidth = width;
    textureInfo.baseTextureHeight = height;
    textureInfo.format.componentCount = components == 4 ?
        TexelComponentCount::QUAD : TexelComponentCount::SINGLE;
    textureInfo.format.componentSize = TexelComponentSize::BYTE;
    textureInfo.format.componentType = TexelComponentType::UNORM;
    textureInfo.mipLevels = 1;
    textureInfo.bindingFlags = TextureBinding::SHADER_RESOURCE;

    toSet = renderer->GetTextureManager()->AddTexture(imageData, textureInfo);

    return toSet != ResourceIndex(-1);
}

bool CreateTransformBuffer(ResourceIndex& toSet,
    Renderer* renderer, float xPos, float yPos, float zPos)
{
    float matrix[16] =
    {
        1.0f, 0.0f, 0.0f, xPos,
        0.0f, 1.0f, 0.0f, yPos,
        0.0f, 0.0f, 1.0f, zPos,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    toSet = renderer->GetBufferManager()->AddBuffer(matrix,
        sizeof(float) * 16, 1, PerFrameWritePattern::ONCE,
        PerFrameWritePattern::NEVER, BufferBinding::CONSTANT_BUFFER);

    return toSet != ResourceIndex(-1);
}

void TransformCamera(Camera* camera, float moveSpeed,
    float turnSpeed, float deltaTime)
{
    if (globalInputs.moveRightPushed)
        camera->MoveRight(moveSpeed * deltaTime);
    else if (globalInputs.moveLeftPushed)
        camera->MoveRight(-moveSpeed * deltaTime);

    if (globalInputs.moveForwardPushed)
        camera->MoveForward(moveSpeed * deltaTime);
    else if (globalInputs.moveBackwardsPushed)
        camera->MoveForward(-moveSpeed * deltaTime);

    if (globalInputs.moveUpPushed)
        camera->MoveUp(moveSpeed * deltaTime);
    else if (globalInputs.moveDownPushed)
        camera->MoveUp(-moveSpeed * deltaTime);

    if (globalInputs.turnLeftPushed)
        camera->RotateY(-turnSpeed * deltaTime);
    else if (globalInputs.turnRightPushed)
        camera->RotateY(turnSpeed * deltaTime);
}

bool LoadSurfacePropertyFiles(SurfaceProperty& surfaceProperties,
    Renderer* renderer, const std::string& prefix)
{
    ResourceIndex diffuseTextureIndex;
    if (!LoadTexture(diffuseTextureIndex, renderer, prefix + "Diffuse.png", 4))
        return false;

    ResourceIndex specularTextureIndex;
    if (!LoadTexture(specularTextureIndex, renderer, prefix + "Specular.png", 4))
        return false;

    surfaceProperties.SetDiffuseTexture(diffuseTextureIndex);
    surfaceProperties.SetSpecularTexture(specularTextureIndex);

    return true;
}

bool CreateLights(ResourceIndex& toSet, Renderer* renderer, float offset)
{
    float height = offset / 2.0f;
    PointLight lights[4] =
    {
        {{ -offset, height, 0.0f }, { 1.0f, 0.0f, 0.0f }},
        {{ offset, height, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        {{ 0.0f, height, -offset }, { 1.0f, 1.0f, 1.0f }},
        {{ 0.0f, height, offset }, { 0.0f, 1.0f, 0.0f }}
    };

    toSet = renderer->GetBufferManager()->AddBuffer(lights,
        sizeof(PointLight), 4, PerFrameWritePattern::NEVER,
        PerFrameWritePattern::NEVER, BufferBinding::STRUCTURED_BUFFER);

    return toSet != ResourceIndex(-1);
}

bool PlacePyramid(const Mesh& cubeMesh, const SurfaceProperty& stoneProperties,
    std::vector<RenderObject>& toStoreIn, Renderer* renderer, int height)
{
    int base = (height - 1) * 2 + 1;

    for (int level = 0; level < height - 1; ++level)
    {
        int topLeftX = -(height - level - 1);
        int topLeftZ = (height - level - 1);
        for (int row = 0; row < base - level * 2; ++row)
        {
            ResourceIndex transformBuffer;
            bool result = CreateTransformBuffer(transformBuffer, renderer,
                topLeftX + row, level + 1, topLeftZ);

            if (result == false)
                return false;

            RenderObject toStore;
            toStore.Initialise(transformBuffer, cubeMesh, stoneProperties);
            toStoreIn.push_back(toStore);

            result = CreateTransformBuffer(transformBuffer, renderer,
                topLeftX + row, level + 1, -topLeftZ);

            if (result == false)
                return false;

            toStore.Initialise(transformBuffer, cubeMesh, stoneProperties);
            toStoreIn.push_back(toStore);
        }

        for (int column = 1; column < base - level * 2 - 1; ++column)
        {
            ResourceIndex transformBuffer;
            bool result = CreateTransformBuffer(transformBuffer, renderer,
                topLeftX, level + 1, topLeftZ - column);

            if (result == false)
                return false;

            RenderObject toStore;
            toStore.Initialise(transformBuffer, cubeMesh, stoneProperties);
            toStoreIn.push_back(toStore);

            result = CreateTransformBuffer(transformBuffer, renderer,
                -topLeftX, level + 1, topLeftZ - column);

            if (result == false)
                return false;

            toStore.Initialise(transformBuffer, cubeMesh, stoneProperties);
            toStoreIn.push_back(toStore);
        }
    }

    ResourceIndex transformBuffer;
    bool result = CreateTransformBuffer(transformBuffer, renderer,
        0, height, 0);

    if (result == false)
        return false;

    RenderObject toStore;
    toStore.Initialise(transformBuffer, cubeMesh, stoneProperties);
    toStoreIn.push_back(toStore);

    return true;
}

bool PlaceGround(const Mesh& cubeMesh, const SurfaceProperty& grassProperties,
    std::vector<RenderObject>& toStoreIn, Renderer* renderer, int height)
{
    height += 2;
    int base = (height - 1) * 2 + 1;

    for (int level = 0; level < height - 1; ++level)
    {
        int topLeftX = -(height - level - 1);
        int topLeftZ = (height - level - 1);

        for (int column = 0; column < base - level * 2; ++column)
        {
            for (int row = 0; row < base - level * 2; ++row)
            {
                ResourceIndex transformBuffer;
                bool result = CreateTransformBuffer(transformBuffer, renderer,
                    topLeftX + column, 0, topLeftZ - row);

                if (result == false)
                    return false;

                RenderObject toStore;
                toStore.Initialise(transformBuffer, cubeMesh, grassProperties);
                toStoreIn.push_back(toStore);
            }
        }
    }

    return true;
}

bool PlaceBlocks(std::vector<RenderObject>& toStoreIn, Renderer* renderer, int height)
{
    Mesh cubeMesh;
    if (!CreateCubeMesh(cubeMesh, renderer))
        return -1;

    SurfaceProperty stoneProperties;
    if (!LoadSurfacePropertyFiles(stoneProperties, renderer, "Stone"))
        return -1;

    SurfaceProperty grassProperties;
    if (!LoadSurfacePropertyFiles(grassProperties, renderer, "Grass"))
        return -1;

    return PlacePyramid(cubeMesh, stoneProperties, toStoreIn, renderer, height)
        && PlaceGround(cubeMesh, grassProperties, toStoreIn, renderer, height);
}

int main(int argc, char* argv[]) {
    const unsigned int WINDOW_WIDTH = 1280;
    const unsigned int WINDOW_HEIGHT = 720;
    SDL_Window* windowHandle = InitialiseWindow(WINDOW_WIDTH, WINDOW_HEIGHT);

    Renderer* renderer;
#ifdef USE_VULKAN
    renderer = new RendererVulkan(windowHandle);
#elif USE_D3D11
    renderer = new RendererD3D11(windowHandle);
#endif

    GraphicsRenderPass* standardPass = CreateStandardRenderPass(renderer);

    const int DIMENSION = 5;
    std::vector<RenderObject> renderObjects;
    if (!PlaceBlocks(renderObjects, renderer, DIMENSION))
        return -1;

    Camera* camera = renderer->CreateCamera(0.1f, 20.0f,
        static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT);
    camera->MoveForward(-DIMENSION);
    camera->MoveUp(1);

    ResourceIndex lightBufferIndex;
    if (!CreateLights(lightBufferIndex, renderer, DIMENSION * 2.5f))
        return -1;

    renderer->SetLightBuffer(lightBufferIndex);

    float deltaTime = 0.0f;
    float moveSpeed = 2.0f;
    float turnSpeed = 3.14f / 2;
    auto lastFrameEnd = std::chrono::system_clock::now();

    SDL_Event event;
    bool run = true;
    while (!globalInputs.quitKey && run)
    {
        if (SDL_PollEvent(&event))
        {
            switch (event.type) {
            case SDL_QUIT:
                run = false;
                break;
            case SDL_KEYDOWN:
                HandleKeyEvent(event.key.keysym, true);
                break;
            case SDL_KEYUP:
                HandleKeyEvent(event.key.keysym, false);
                break;
            default:
                break;
            }
        }
        else
        {

            TransformCamera(camera, moveSpeed, turnSpeed, deltaTime);

            renderer->PreRender();

            renderer->SetCamera(camera);
            renderer->SetRenderPass(standardPass);
            renderer->Render(renderObjects);

            renderer->Present();
            auto currentFrameEnd = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<
                std::chrono::microseconds>(currentFrameEnd - lastFrameEnd).count();
            deltaTime = elapsed / 1000000.0f;
            lastFrameEnd = currentFrameEnd;
        }

    }

    renderer->DestroyGraphicsRenderPass(standardPass);
    delete renderer;
    SDL_Quit();

    return 0;
}

cmake_minimum_required(VERSION 3.21)

project(GridRenderer)

# Choose one :)
#set(RENDER_BACKEND VULKAN)
set(RENDER_BACKEND D3D11)

if (NOT DEFINED RENDER_BACKEND)
    message(FATAL_ERROR "One render backend must be selected")
else ()
    add_compile_definitions(USE_${RENDER_BACKEND})
endif ()

# General project properties
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_compile_definitions(
        SHADER_ROOT_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        CONTENT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Textures/")

# Dependencies
if (RENDER_BACKEND STREQUAL "D3D11")
    find_program(FXC fxc REQUIRED)
elseif (RENDER_BACKEND STREQUAL "VULKAN")
    find_package(Vulkan REQUIRED)

    if (NOT DEFINED Vulkan_GLSLC_EXECUTABLE)
        message(FATAL_ERROR "Couldn't find glslc")
    endif()
endif ()
find_package(SDL2 CONFIG REQUIRED)

# HLSL shader compilation with FXC
if (RENDER_BACKEND STREQUAL "D3D11")
    add_custom_target(shaders)

    set(SHADER_SRC_FILES
            StandardPS.hlsl
            StandardVS.hlsl)
    set_source_files_properties(StandardVS.hlsl PROPERTIES shader_type "vs")
    set_source_files_properties(StandardPS.hlsl PROPERTIES shader_type "ps")

    foreach (SHADER_FILE ${SHADER_SRC_FILES})
        get_source_file_property(SHADER_TYPE ${SHADER_FILE} shader_type)
        get_filename_component(OUT_NAME ${SHADER_FILE} NAME_WLE)

        if (CMAKE_BUILD_TYPE MATCHES "Debug")
            set(FXC_DEBUG_FLAGS /Od /Zi)
        endif ()

        set(OUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${OUT_NAME}.cso)
        set(COMPILE_COMMAND ${FXC} ${FXC_DEBUG_FLAGS} /T ${SHADER_TYPE}_5_0 /Fo ${OUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/GridRenderer/D3D11/${SHADER_FILE})
        message(${COMPILE_COMMAND})

        add_custom_command(
                TARGET shaders
                MAIN_DEPENDENCY ${OUT_PATH}
                COMMENT "Compiling ${OUT_PATH}"
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
                COMMAND ${COMPILE_COMMAND}
                VERBATIM
        )
    endforeach (SHADER_FILE)
elseif (RENDER_BACKEND STREQUAL "VULKAN")
    add_custom_target(shaders)

    set(SHADER_SRC_FILES
            Standard.vert
            Standard.frag)

    foreach (SHADER_FILE ${SHADER_SRC_FILES})
        get_filename_component(OUT_NAME ${SHADER_FILE} NAME)

        if (CMAKE_BUILD_TYPE MATCHES "Debug")
            set(GLSLC_DEBUG_FLAGS -O0 -g)
        endif ()

        set(OUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${OUT_NAME}.spv)
        set(COMPILE_COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSLC_DEBUG_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/GridRenderer/Vulkan/${SHADER_FILE} -o ${OUT_PATH})
        message(${COMPILE_COMMAND})

        add_custom_command(
                TARGET shaders
                MAIN_DEPENDENCY ${OUT_PATH}
                COMMENT "Compiling ${OUT_PATH}"
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
                COMMAND ${COMPILE_COMMAND}
                VERBATIM
        )
    endforeach (SHADER_FILE)
endif ()

# General compilation
# Use list transforms to avoid having to add GridRenderer/ in front of all source files
set(SRC_ROOT_DIR GridRenderer/)

set(SRC_FILES
        Main.cpp
        Mesh.cpp
        RenderObject.cpp
        SurfaceProperty.cpp)
list(TRANSFORM SRC_FILES PREPEND ${SRC_ROOT_DIR})

if (RENDER_BACKEND STREQUAL "D3D11")
    set(D3D11_SRC_FILES
            BufferManagerD3D11.cpp
            CameraD3D11.cpp
            GraphicsRenderPassD3D11.cpp
            RendererD3D11.cpp
            SamplerManagerD3D11.cpp
            TextureManagerD3D11.cpp)
    list(TRANSFORM D3D11_SRC_FILES PREPEND ${SRC_ROOT_DIR}D3D11/)
else ()
    set(VULKAN_SRC_FILES
            RendererVulkan.cpp
            SamplerManagerVulkan.cpp
            GraphicsRenderPassVulkan.cpp
            BufferManagerVulkan.cpp
            TextureManagerVulkan.cpp
            FileUtils.cpp)
    list(TRANSFORM VULKAN_SRC_FILES PREPEND ${SRC_ROOT_DIR}Vulkan/)
endif ()

add_executable(GridRenderer ${SRC_FILES} ${D3D11_SRC_FILES} ${VULKAN_SRC_FILES})

# https://stackoverflow.com/questions/2368811/how-to-set-warning-level-in-cmake
if (MSVC)
    target_compile_options(GridRenderer PRIVATE /W4)
else ()
    target_compile_options(GridRenderer PRIVATE -Wall -Wextra -Wpedantic)
endif ()

add_dependencies(GridRenderer shaders)

if (RENDER_BACKEND STREQUAL "D3D11")
    set(LINK_LIBRARIES d3d11.lib d3dcompiler.lib)
elseif (RENDER_BACKEND STREQUAL "VULKAN")
    set(LINK_LIBRARIES ${Vulkan_LIBRARIES})

    target_include_directories(GridRenderer PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_compile_definitions(GridRenderer PRIVATE
            VULKAN_HPP_NO_CONSTRUCTORS # Prefer designated initializers over constructors
            # The code base uses exceptions, but if not using exceptions this should be defined
            # VULKAN_HPP_NO_EXCEPTIONS
            # This might be needed to enable resizing
            # VULKAN_HPP_ASSERT_ON_RESULT=\(void\)
            )
endif ()

target_link_libraries(GridRenderer PRIVATE SDL2::SDL2 SDL2::SDL2main ${LINK_LIBRARIES})
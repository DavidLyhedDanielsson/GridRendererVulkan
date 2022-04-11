cmake_minimum_required(VERSION 3.21)

project(GridRenderer)

# General project properties
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_compile_definitions(SHADER_ROOT_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
add_compile_definitions(CONTENT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Textures/")

# Dependencies
find_program(FXC fxc REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# HLSL shader compilation with FXC
add_custom_target(shaders)

set(SHADER_SRC_FILES
    StandardPS.hlsl
    StandardVS.hlsl)
set_source_files_properties(StandardVS.hlsl PROPERTIES shader_type "vs")
set_source_files_properties(StandardPS.hlsl PROPERTIES shader_type "ps")

foreach(SHADER_FILE ${SHADER_SRC_FILES})
    get_source_file_property(SHADER_TYPE ${SHADER_FILE} shader_type)
    get_filename_component(OUT_NAME ${SHADER_FILE} NAME_WLE)

    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(FXC_DEBUG_FLAGS /Od /Zi)
    endif()

    set(OUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${OUT_NAME}.cso)
    set(COMPILE_COMMAND ${FXC} ${FXC_DEBUG_FLAGS} /T ${SHADER_TYPE}_5_0 /Fo ${OUT_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/GridRenderer/${SHADER_FILE})
    message(${COMPILE_COMMAND})
    
    add_custom_command(
        TARGET shaders
        MAIN_DEPENDENCY ${OUT_PATH}
        COMMENT "Compiling ${OUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
        COMMAND ${COMPILE_COMMAND}
        VERBATIM
    )
endforeach(SHADER_FILE)

# General compilation
# Use list transforms to avoid having to add GridRenderer/ in front of all source files
set(SRC_ROOT_DIR GridRenderer/)

set(SRC_FILES
        Main.cpp
        Mesh.cpp
        RenderObject.cpp
        SurfaceProperty.cpp)
list(TRANSFORM SRC_FILES PREPEND ${SRC_ROOT_DIR})

set(D3D11_SRC_FILES
        BufferManagerD3D11.cpp
        CameraD3D11.cpp
        GraphicsRenderPassD3D11.cpp
        RendererD3D11.cpp
        SamplerManagerD3D11.cpp
        TextureManagerD3D11.cpp)
list(TRANSFORM D3D11_SRC_FILES PREPEND ${SRC_ROOT_DIR})

add_executable(GridRenderer ${SRC_FILES} ${D3D11_SRC_FILES})
add_dependencies(GridRenderer shaders)

target_link_libraries(GridRenderer PRIVATE SDL2::SDL2 SDL2::SDL2main d3d11.lib d3dcompiler.lib)
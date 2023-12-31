cmake_minimum_required(VERSION 3.22)
set(LIB_NAME GpuVk)
project(${LIB_NAME})

set(CMAKE_CXX_STANDARD 20)

# Find packages, the following line is only necessary for VCPKG users.
set(VCPKG_TARGET_TRIPLET x64-windows-static)
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

if (MSVC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

add_library(
        ${LIB_NAME} STATIC
        src/GpuVk/RenderEngine.cpp src/GpuVk/RenderEngine.hpp
        src/GpuVk/Buffer.cpp src/GpuVk/Buffer.hpp
        src/GpuVk/Commands.cpp src/GpuVk/Commands.hpp
        src/GpuVk/Swapchain.cpp src/GpuVk/Swapchain.hpp
        src/GpuVk/Image.cpp src/GpuVk/Image.hpp
        src/GpuVk/Pipeline.cpp src/GpuVk/Pipeline.hpp
        src/GpuVk/RenderPass.cpp src/GpuVk/RenderPass.hpp
        src/GpuVk/Gpu.cpp src/GpuVk/Gpu.hpp
        src/GpuVk/Sampler.cpp src/GpuVk/Sampler.hpp
        src/GpuVk/File.hpp src/GpuVk/File.cpp
        src/GpuVk/UniformBuffer.hpp
        src/GpuVk/Model.hpp
        src/GpuVk/QueueFamilyIndices.hpp
        src/GpuVk/HeaderImpls.cpp
        src/GpuVk/IRenderer.hpp
        src/GpuVk/Constants.hpp
        src/GpuVk/PipelineOptions.hpp
        src/GpuVk/Format.hpp
        src/GpuVk/RenderPassOptions.hpp
        src/GpuVk/FilterMode.hpp
        src/GpuVk/PresentMode.hpp)

target_include_directories(${LIB_NAME} INTERFACE src/GpuVk/..)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)

set(
        LINK_LIBRARIES
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
        Vulkan::Vulkan
        glm::glm
        unofficial::vulkan-memory-allocator::vulkan-memory-allocator
)

target_link_libraries(${LIB_NAME} PRIVATE ${LINK_LIBRARIES})

# Examples:
set(EXAMPLE_NAMES UpdateExample CubesExample RenderTextureExample 2dExample)

foreach (EXAMPLE IN LISTS EXAMPLE_NAMES)
    add_executable(${EXAMPLE} src/Examples/${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE} PRIVATE ${LIB_NAME} ${LINK_LIBRARIES})
endforeach ()

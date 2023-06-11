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
endif()

add_library(
        ${LIB_NAME} STATIC
        src/vkFrame/renderer.cpp src/vkFrame/renderer.hpp
        src/vkFrame/buffer.cpp src/vkFrame/buffer.hpp
        src/vkFrame/commands.cpp src/vkFrame/commands.hpp
        src/vkFrame/swapchain.cpp src/vkFrame/swapchain.hpp
        src/vkFrame/image.cpp src/vkFrame/image.hpp
        src/vkFrame/pipeline.cpp src/vkFrame/pipeline.hpp
        src/vkFrame/renderPass.cpp src/vkFrame/renderPass.hpp
        src/vkFrame/uniformBuffer.hpp
        src/vkFrame/model.hpp
        src/vkFrame/queueFamilyIndices.hpp
        src/vkFrame/headerImpls.cpp
)

target_include_directories(${LIB_NAME} INTERFACE src/vkFrame/..)

find_package(SDL2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)

target_link_libraries(
        ${LIB_NAME} PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        Vulkan::Vulkan
        glm::glm
        unofficial::vulkan-memory-allocator::vulkan-memory-allocator
)

# Examples:
set(ExampleNames UpdateExample CubesExample RenderTextureExample 2dExample)

add_executable(UpdateExample src/examples/update.cpp)
add_executable(CubesExample src/examples/cubes.cpp)
add_executable(RenderTextureExample src/examples/renderTexture.cpp)
add_executable(2dExample src/examples/2d.cpp)

foreach(EXAMPLE IN LISTS ExampleNames)
    target_link_libraries(
            ${EXAMPLE} PRIVATE
            ${LIB_NAME}
            $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
            $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
            Vulkan::Vulkan
            glm::glm
            unofficial::vulkan-memory-allocator::vulkan-memory-allocator
    )
endforeach()

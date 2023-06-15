#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include "Buffer.hpp"
#include "Commands.hpp"
#include "IRenderer.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "QueueFamilyIndices.hpp"
#include "Swapchain.hpp"
#include "UniformBuffer.hpp"

namespace GpuVk
{
class RenderEngine
{
    public:
    template <class T>
    typename std::enable_if<std::is_base_of<IRenderer, T>::value>::type Run(const std::string& windowTitle,
        const uint32_t windowWidth, const uint32_t windowHeight, T renderer,
        PresentMode preferredPresentMode = PresentMode::Vsync)
    {
        InitWindow(windowTitle, windowWidth, windowHeight);
        InitVulkan(preferredPresentMode, windowWidth, windowHeight);
        renderer.Init(_gpu, _window, windowWidth, windowHeight);
        MainLoop(renderer);
        // Destruct the renderer before cleaning up the resources it may be using.
        {
            auto _ = std::move(renderer);
        }
        Cleanup(renderer);
    }

    private:
    SDL_Window* _window;

    std::shared_ptr<Gpu> _gpu;

    bool _framebufferResized = false;

    void InitWindow(const std::string& windowTitle, const uint32_t windowWidth, const uint32_t windowHeight);
    void InitVulkan(PresentMode preferredPresentMode, const uint32_t windowWidth, const uint32_t windowHeight);

    void MainLoop(IRenderer&);
    void DrawFrame(IRenderer&);
    void WaitWhileMinimized();
    void Cleanup(IRenderer&);
};
} // namespace GpuVk
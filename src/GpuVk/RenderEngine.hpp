#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../deps/stb_image.h"

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

class RenderEngine
{
    public:
    void Run(
        const std::string& windowTitle, const uint32_t windowWidth, const uint32_t windowHeight, IRenderer& renderer);

    private:
    SDL_Window* _window;

    VulkanState _vulkanState;

    bool _framebufferResized = false;

    void InitWindow(const std::string& windowTitle, const uint32_t windowWidth, const uint32_t windowHeight);
    void InitVulkan(IRenderer&);

    void MainLoop(IRenderer&);
    void DrawFrame(IRenderer&);
    void WaitWhileMinimized();
    void Cleanup(IRenderer&);
};

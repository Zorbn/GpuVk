#pragma once

#include "VulkanState.hpp"

class IRenderer
{
    public:
    virtual ~IRenderer()
    {
    }

    virtual void Init(Gpu& gpu, VulkanState& vulkanState, SDL_Window* window, int32_t width, int32_t height) = 0;
    virtual void Update(Gpu& gpu) = 0;
    virtual void Render(Gpu& gpu) = 0;
    virtual void Resize(Gpu& gpu, VulkanState& vulkanState, int32_t width, int32_t height) = 0;
    virtual void Cleanup(Gpu& gpu, VulkanState& vulkanState) = 0;
};
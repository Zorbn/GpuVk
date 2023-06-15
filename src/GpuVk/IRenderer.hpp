#pragma once

#include "Gpu.hpp"

namespace GpuVk
{
class IRenderer
{
    public:
    virtual ~IRenderer()
    {
    }

    virtual void Init(std::shared_ptr<Gpu> gpu, SDL_Window* window, int32_t width, int32_t height) = 0;
    virtual void Update(std::shared_ptr<Gpu> gpu) = 0;
    virtual void Render(std::shared_ptr<Gpu> gpu) = 0;
    virtual void Resize(std::shared_ptr<Gpu> gpu, int32_t width, int32_t height) = 0;
};
} // namespace GpuVk
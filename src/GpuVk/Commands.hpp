#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "QueueFamilyIndices.hpp"

namespace GpuVk
{
class Gpu;

class Commands
{
    friend class RenderEngine;
    friend class Gpu;
    friend class Buffer;
    friend class Image;
    friend class Pipeline;
    friend class RenderPass;
    template <typename V, typename I, typename D> friend class Model;

    public:
    void BeginBuffer();
    void EndBuffer();

    private:
    Commands() = default;
    Commands(std::shared_ptr<Gpu> gpu);
    Commands(Commands&& other);
    Commands& operator=(Commands&& other);
    ~Commands();

    void CreatePool();
    void CreateBuffers();
    void ResetBuffer();

    const VkCommandBuffer& GetBuffer() const;

    VkCommandBuffer BeginSingleTime() const;
    void EndSingleTime(VkCommandBuffer commandBuffer) const;

    std::shared_ptr<Gpu> _gpu;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _buffers;
    uint32_t _currentBufferIndex = 0;
};
} // namespace GpuVk
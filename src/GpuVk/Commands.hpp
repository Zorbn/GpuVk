#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

#include "QueueFamilyIndices.hpp"

class Gpu;

class Commands
{
    friend class RenderEngine;
    friend class Gpu;

    public:
    VkCommandBuffer BeginSingleTime() const;
    void EndSingleTime(VkCommandBuffer commandBuffer) const;

    void BeginBuffer();
    void EndBuffer();
    const VkCommandBuffer& GetBuffer() const;

    void SetCurrentBufferIndex(uint32_t currentBufferIndex);
    uint32_t GetCurrentBufferIndex() const;

    private:
    Commands() = default;
    Commands(std::shared_ptr<Gpu> gpu);
    Commands(Commands&& other);
    Commands& operator=(Commands&& other);
    ~Commands();

    void CreatePool();
    void CreateBuffers();
    void ResetBuffer();

    std::shared_ptr<Gpu> _gpu;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _buffers;
    uint32_t _currentBufferIndex = 0;
};
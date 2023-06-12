#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

#include "QueueFamilyIndices.hpp"

class Commands
{
    public:
    VkCommandBuffer BeginSingleTime(VkDevice device) const;
    void EndSingleTime(VkCommandBuffer commandBuffer, VkQueue graphicsQueue, VkDevice device) const;

    void CreatePool(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface);

    void CreateBuffers(VkDevice device);
    void ResetBuffer();
    void BeginBuffer();
    void EndBuffer();
    const VkCommandBuffer& GetBuffer() const;

    void Destroy(VkDevice device);

    void SetCurrentBufferIndex(uint32_t currentBufferIndex);
    uint32_t GetCurrentBufferIndex() const;

    private:
    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _buffers;
    uint32_t _currentBufferIndex = 0;
};
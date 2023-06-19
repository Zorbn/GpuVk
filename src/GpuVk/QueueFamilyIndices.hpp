#pragma once

#include <optional>

namespace GpuVk
{
class QueueFamilyIndices
{
    friend class Commands;
    friend class Gpu;
    friend class Swapchain;

    private:
    std::optional<uint32_t> _graphicsFamily;
    std::optional<uint32_t> _presentFamily;

    QueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                _graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
                _presentFamily = i;
            if (IsComplete())
                break;

            i++;
        }
    }

    bool IsComplete()
    {
        return _graphicsFamily.has_value() && _presentFamily.has_value();
    }
};
} // namespace GpuVk
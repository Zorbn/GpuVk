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

    bool IsComplete()
    {
        return _graphicsFamily.has_value() && _presentFamily.has_value();
    }

    static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices._graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
                indices._presentFamily = i;
            if (indices.IsComplete())
                break;

            i++;
        }

        return indices;
    }
};
} // namespace GpuVk
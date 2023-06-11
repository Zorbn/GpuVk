#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Image.hpp"
#include "QueueFamilyIndices.hpp"

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

class Swapchain
{
    public:
    void Create(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int32_t windowWidth,
        int32_t windowHeight, VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR);
    void Cleanup(VkDevice device);
    void Recreate(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
        int32_t windowWidth, int32_t windowHeight);

    SwapchainSupportDetails QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChoosePresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes, VkPresentModeKHR preferredPresentMode);
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR &capabilities, int32_t windowWidth, int32_t windowHeight);
    VkResult GetNextImage(VkDevice device, VkSemaphore semaphore, uint32_t &imageIndex);

    const VkSwapchainKHR &GetSwapchain();
    const VkExtent2D &GetExtent();
    const VkFormat &GetImageFormat();

    private:
    VkSwapchainKHR _swapchain;
    VkExtent2D _extent;
    VkFormat _imageFormat;
};
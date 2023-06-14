#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

#include "Image.hpp"
#include "QueueFamilyIndices.hpp"

class Gpu;

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

class Swapchain
{
    friend class Gpu;
    friend class RenderEngine;

    public:
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChoosePresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR preferredPresentMode);
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, int32_t windowWidth, int32_t windowHeight);
    VkResult GetNextImage();

    const VkSwapchainKHR& GetSwapchain() const;
    const VkExtent2D& GetExtent() const;
    const VkFormat& GetImageFormat() const;
    const uint32_t& GetCurrentImageIndex() const;

    private:
    Swapchain() = default;
    Swapchain(std::shared_ptr<Gpu> gpu, int32_t windowWidth, int32_t windowHeight,
        VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR);
    Swapchain(Swapchain&& other);
    Swapchain& operator=(Swapchain&& other);
    ~Swapchain();

    void Create(int32_t windowWidth, int32_t windowHeight);
    void Destroy();
    void Resize(int32_t windowWidth, int32_t windowHeight);

    static SwapchainSupportDetails QuerySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    std::shared_ptr<Gpu> _gpu;

    VkSwapchainKHR _swapchain;
    VkPresentModeKHR _preferredPresentMode;
    VkExtent2D _extent;
    VkFormat _imageFormat;
    uint32_t _currentImageIndex = 0;
};
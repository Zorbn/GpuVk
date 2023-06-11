#include "Swapchain.hpp"

void Swapchain::Create(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int32_t windowWidth,
    int32_t windowHeight, VkPresentModeKHR preferredPresentMode)
{
    SwapchainSupportDetails swapchainSupport = QuerySupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapchainSupport.Formats);
    VkPresentModeKHR presentMode = ChoosePresentMode(swapchainSupport.PresentModes, preferredPresentMode);
    _extent = ChooseExtent(swapchainSupport.Capabilities, windowWidth, windowHeight);

    uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;
    if (swapchainSupport.Capabilities.maxImageCount > 0 && imageCount > swapchainSupport.Capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = _extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    if (indices.GraphicsFamily != indices.PresentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    _imageFormat = surfaceFormat.format;
}

SwapchainSupportDetails Swapchain::QuerySupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::ChoosePresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes, VkPresentModeKHR preferredPresentMode)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == preferredPresentMode)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseExtent(
    const VkSurfaceCapabilitiesKHR &capabilities, int32_t windowWidth, int32_t windowHeight)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = {static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Swapchain::Cleanup(VkDevice device)
{
    vkDestroySwapchainKHR(device, _swapchain, nullptr);
}

void Swapchain::Recreate(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
    int32_t windowWidth, int32_t windowHeight)
{
    vkDeviceWaitIdle(device);

    Cleanup(device);

    Create(device, physicalDevice, surface, windowWidth, windowHeight);
}

VkResult Swapchain::GetNextImage(VkDevice device, VkSemaphore semaphore, uint32_t &imageIndex)
{
    return vkAcquireNextImageKHR(device, _swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);
}

const VkSwapchainKHR &Swapchain::GetSwapchain()
{
    return _swapchain;
}

const VkFormat &Swapchain::GetImageFormat()
{
    return _imageFormat;
}

const VkExtent2D &Swapchain::GetExtent()
{
    return _extent;
}
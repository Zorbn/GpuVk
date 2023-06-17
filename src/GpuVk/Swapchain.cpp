#include "Swapchain.hpp"
#include "Gpu.hpp"

namespace GpuVk
{
Swapchain::Swapchain(
    std::shared_ptr<Gpu> gpu, int32_t windowWidth, int32_t windowHeight, PresentMode preferredPresentMode)
    : _gpu(gpu), _preferredPresentMode(preferredPresentMode)
{
    Create(windowWidth, windowHeight);
}

Swapchain::Swapchain(Swapchain&& other)
{
    *this = std::move(other);
}

Swapchain& Swapchain::operator=(Swapchain&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_swapchain, other._swapchain);
    std::swap(_preferredPresentMode, other._preferredPresentMode);
    std::swap(_extent, other._extent);
    std::swap(_imageFormat, other._imageFormat);
    std::swap(_currentImageIndex, other._currentImageIndex);

    return *this;
}

Swapchain::~Swapchain()
{
    if (!_gpu)
        return;

    Destroy();
}

void Swapchain::Create(int32_t windowWidth, int32_t windowHeight)
{
    auto swapchainSupport = QuerySupport(_gpu->_physicalDevice, _gpu->_surface);

    auto surfaceFormat = ChooseSurfaceFormat(swapchainSupport.Formats);
    auto presentMode = ChoosePresentMode(swapchainSupport.PresentModes, GetVkPresentMode(_preferredPresentMode));
    _extent = ChooseExtent(swapchainSupport.Capabilities, windowWidth, windowHeight);

    uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;
    if (swapchainSupport.Capabilities.maxImageCount > 0 && imageCount > swapchainSupport.Capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _gpu->_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = _extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = QueueFamilyIndices::FindQueueFamilies(_gpu->_physicalDevice, _gpu->_surface);
    uint32_t queueFamilyIndices[] = {indices._graphicsFamily.value(), indices._presentFamily.value()};

    if (indices._graphicsFamily != indices._presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
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

    if (vkCreateSwapchainKHR(_gpu->_device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    _imageFormat = surfaceFormat.format;
}

void Swapchain::Destroy()
{
    vkDestroySwapchainKHR(_gpu->_device, _swapchain, nullptr);
}

void Swapchain::UpdatePresentMode(PresentMode presentMode)
{
    _preferredPresentMode = presentMode;
    Resize(_extent.width, _extent.height);
}

void Swapchain::Resize(int32_t windowWidth, int32_t windowHeight)
{
    vkDeviceWaitIdle(_gpu->_device);

    Destroy();
    Create(windowWidth, windowHeight);
}

SwapchainSupportDetails Swapchain::QuerySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.Capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

VkPresentModeKHR Swapchain::GetVkPresentMode(PresentMode presentMode)
{
    switch (presentMode)
    {
        case PresentMode::Vsync:
            return VK_PRESENT_MODE_MAILBOX_KHR;
        case PresentMode::NoVsync:
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        default:
            throw std::runtime_error("Tried to get a VkPresentModeKHR from an invalid present mode!");
    }
}

VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
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
    const std::vector<VkPresentModeKHR>& availablePresentModes, VkPresentModeKHR preferredPresentMode)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == preferredPresentMode)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseExtent(
    const VkSurfaceCapabilitiesKHR& capabilities, int32_t windowWidth, int32_t windowHeight)
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

VkResult Swapchain::GetNextImage()
{
    return vkAcquireNextImageKHR(_gpu->_device, _swapchain, UINT64_MAX, _gpu->GetCurrentImageAvailableSemaphore(),
        VK_NULL_HANDLE, &_currentImageIndex);
}
} // namespace GpuVk
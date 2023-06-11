#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <functional>
#include <vector>

#include "image.hpp"
#include "swapchain.hpp"

class RenderPass
{
    public:
    void CreateCustom(VkDevice device, Swapchain &swapchain, std::function<VkRenderPass()> setupRenderPass,
        std::function<void(const VkExtent2D &extent)> recreateCallback, std::function<void()> cleanupCallback,
        std::function<void(std::vector<VkImageView> &attachments, VkImageView imageView)> setupFramebuffer);
    void Create(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, Swapchain &swapchain,
        bool enableDepth, bool enableMsaa);
    void Recreate(VkDevice device, Swapchain &swapchain);

    void Begin(const uint32_t imageIndex, VkCommandBuffer commandBuffer, VkExtent2D extent,
        const std::vector<VkClearValue> &clearValues);
    void End(VkCommandBuffer commandBuffer);

    VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates,
        VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

    const VkRenderPass &GetRenderPass();
    const VkFramebuffer &GetFramebuffer(const uint32_t imageIndex);
    const VkSampleCountFlagBits GetMsaaSamples();
    const bool GetMsaaEnabled();

    void Cleanup(VkDevice device);

    private:
    void CreateImages(VkDevice device, Swapchain &swapchain);
    void CreateFramebuffers(VkDevice device, VkExtent2D extent);
    void CreateDepthResources(
        VmaAllocator allocator, VkPhysicalDevice physicalDevice, VkDevice device, VkExtent2D extent);
    void CreateColorResources(VmaAllocator allocator, VkDevice device, VkExtent2D extent);
    void CreateImageViews(VkDevice device);
    void CleanupForRecreation(VkDevice device);

    const VkSampleCountFlagBits GetMaxUsableSamples(VkPhysicalDevice physicalDevice);

    std::function<void()> _cleanupCallback;
    std::function<void(const VkExtent2D &)> _recreateCallback;
    std::function<void(std::vector<VkImageView> &attachments, VkImageView imageView)> _setupFramebuffer;

    VkRenderPass _renderPass;

    std::vector<Image> _images;
    std::vector<VkImageView> _imageViews;
    std::vector<VkFramebuffer> _framebuffers;

    Image _depthImage;
    VkImageView _depthImageView;
    Image _colorImage;
    VkImageView _colorImageView;
    VkFormat _imageFormat;
    bool _depthEnabled = false;
    bool _msaaEnabled = false;
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};
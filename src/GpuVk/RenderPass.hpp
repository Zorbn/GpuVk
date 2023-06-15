#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <functional>
#include <vector>

#include "Image.hpp"
#include "Gpu.hpp"
#include "RenderPassOptions.hpp"

class RenderPass
{
    public:
    RenderPass() = default;
    RenderPass(std::shared_ptr<Gpu> gpu, RenderPassOptions renderPassOptions);
    RenderPass(RenderPass&& other);
    RenderPass& operator=(RenderPass&& other);
    ~RenderPass();

    void Begin(const ClearColor& clearColor);
    void End();

    const VkRenderPass& GetRenderPass() const;
    const VkSampleCountFlagBits GetMsaaSamples() const;
    const bool IsUsingMsaa() const;
    const Image& GetColorImage() const;

    void UpdateResources();

    private:
    void Create();
    void CreateImages();
    void CreateFramebuffers();
    void CreateDepthResources();
    void CreateColorResources();
    void CleanupResources();

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
        VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat FindDepthFormat();

    const VkSampleCountFlagBits GetMaxUsableSamples(VkPhysicalDevice physicalDevice);

    std::shared_ptr<Gpu> _gpu;

    RenderPassOptions _options;

    VkRenderPass _renderPass;

    std::vector<Image> _images;
    std::vector<VkFramebuffer> _framebuffers;

    Image _depthImage;
    Image _colorImage;
    VkFormat _imageFormat;
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};
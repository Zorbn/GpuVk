#include "renderPass.hpp"
#include "swapchain.hpp"

void RenderPass::CreateCustom(VkDevice device, Swapchain &swapchain, std::function<VkRenderPass()> setupRenderPass,
    std::function<void(const VkExtent2D &extent)> recreateCallback, std::function<void()> cleanupCallback,
    std::function<void(std::vector<VkImageView> &attachments, VkImageView imageView)> setupFramebuffer)
{

    _imageFormat = swapchain.GetImageFormat();

    _cleanupCallback = cleanupCallback;
    _recreateCallback = recreateCallback;
    _setupFramebuffer = setupFramebuffer;

    _renderPass = setupRenderPass();

    CreateImages(device, swapchain);
    CreateImageViews(device);

    const VkExtent2D &extent = swapchain.GetExtent();
    recreateCallback(extent);
    CreateFramebuffers(device, extent);
}

void RenderPass::Create(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator allocator, Swapchain &swapchain,
    bool enableDepth, bool enableMsaa)
{
    std::function<VkRenderPass()> setupRenderPass = [&]
    {
        _depthEnabled = enableDepth;
        _msaaSamples = enableMsaa ? GetMaxUsableSamples(physicalDevice) : VK_SAMPLE_COUNT_1_BIT;
        _msaaEnabled = _msaaSamples != VK_SAMPLE_COUNT_1_BIT;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = _imageFormat;
        colorAttachment.samples = _msaaSamples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout =
            _msaaEnabled ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = FindDepthFormat(physicalDevice);
        depthAttachment.samples = _msaaSamples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachmentResolve{};
        colorAttachmentResolve.format = _imageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentResolveRef{};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        if (_msaaEnabled)
        {
            subpass.pResolveAttachments = &colorAttachmentResolveRef;
        }

        if (_depthEnabled)
        {
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
        }

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment};

        if (_msaaEnabled)
        {
            attachments.push_back(colorAttachmentResolve);
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkRenderPass renderPass;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass!");
        }

        return renderPass;
    };

    std::function<void(const VkExtent2D &)> recreateCallback = [=](const VkExtent2D &extent)
    {
        CreateColorResources(allocator, device, extent);
        CreateDepthResources(allocator, physicalDevice, device, extent);
    };

    std::function<void()> cleanupCallback = [=]
    {
        vkDestroyImageView(device, _depthImageView, nullptr);
        _depthImage.Destroy(allocator);
        vkDestroyImageView(device, _colorImageView, nullptr);
        _colorImage.Destroy(allocator);
    };

    std::function<void(std::vector<VkImageView> &, VkImageView)> setupFramebuffer =
        [&](std::vector<VkImageView> &attachments, VkImageView imageView)
    {
        if (_msaaEnabled)
        {
            attachments.push_back(_colorImageView);
        }
        else
        {
            attachments.push_back(imageView);
        }

        attachments.push_back(_depthImageView);

        if (_msaaEnabled)
        {
            attachments.push_back(imageView);
        }
    };

    CreateCustom(device, swapchain, setupRenderPass, recreateCallback, cleanupCallback, setupFramebuffer);
}

void RenderPass::CreateImages(VkDevice device, Swapchain &swapchain)
{
    VkSwapchainKHR vkSwapchain = swapchain.GetSwapchain();
    VkFormat format = swapchain.GetImageFormat();
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, vkSwapchain, &imageCount, nullptr);
    std::vector<VkImage> imagesVk;
    imagesVk.resize(imageCount);
    _images.clear();
    _images.reserve(imageCount);
    vkGetSwapchainImagesKHR(device, vkSwapchain, &imageCount, imagesVk.data());

    for (VkImage vkImage : imagesVk)
    {
        _images.push_back(Image(vkImage, format));
    }
}

void RenderPass::Begin(const uint32_t imageIndex, VkCommandBuffer commandBuffer, VkExtent2D extent,
    const std::vector<VkClearValue> &clearValues)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void RenderPass::End(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

const VkRenderPass &RenderPass::GetRenderPass()
{
    return _renderPass;
}

const VkSampleCountFlagBits RenderPass::GetMsaaSamples()
{
    return _msaaSamples;
}

const bool RenderPass::GetMsaaEnabled()
{
    return _msaaEnabled;
}

void RenderPass::CreateImageViews(VkDevice device)
{
    _imageViews.resize(_images.size());

    for (uint32_t i = 0; i < _images.size(); i++)
    {
        _imageViews[i] = _images[i].CreateView(VK_IMAGE_ASPECT_COLOR_BIT, device);
    }
}

void RenderPass::CreateFramebuffers(VkDevice device, VkExtent2D extent)
{
    _framebuffers.resize(_imageViews.size());

    for (size_t i = 0; i < _imageViews.size(); i++)
    {
        std::vector<VkImageView> attachments;
        _setupFramebuffer(attachments, _imageViews[i]);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void RenderPass::CreateDepthResources(
    VmaAllocator allocator, VkPhysicalDevice physicalDevice, VkDevice device, VkExtent2D extent)
{
    VkFormat depthFormat = FindDepthFormat(physicalDevice);

    _depthImage = Image(allocator, extent.width, extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 1, _msaaSamples);
    _depthImageView = _depthImage.CreateView(VK_IMAGE_ASPECT_DEPTH_BIT, device);
}

void RenderPass::CreateColorResources(VmaAllocator allocator, VkDevice device, VkExtent2D extent)
{
    _colorImage = Image(allocator, extent.width, extent.height, _imageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1, 1, _msaaSamples);
    _colorImageView = _colorImage.CreateView(VK_IMAGE_ASPECT_COLOR_BIT, device);
}

void RenderPass::Recreate(VkDevice device, Swapchain &swapchain)
{
    CleanupForRecreation(device);

    const VkExtent2D &extent = swapchain.GetExtent();

    CreateImages(device, swapchain);
    CreateImageViews(device);
    _recreateCallback(extent);
    CreateFramebuffers(device, extent);
}

VkFormat RenderPass::FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

VkFormat RenderPass::FindDepthFormat(VkPhysicalDevice physicalDevice)
{
    return FindSupportedFormat(physicalDevice,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void RenderPass::CleanupForRecreation(VkDevice device)
{
    _cleanupCallback();

    for (auto framebuffer : _framebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : _imageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
}

void RenderPass::Cleanup(VkDevice device)
{
    CleanupForRecreation(device);
    vkDestroyRenderPass(device, _renderPass, nullptr);
}

const VkFramebuffer &RenderPass::GetFramebuffer(const uint32_t imageIndex)
{
    return _framebuffers[imageIndex];
}

const VkSampleCountFlagBits RenderPass::GetMaxUsableSamples(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT)
        return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT)
        return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT)
        return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT)
        return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT)
        return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT)
        return VK_SAMPLE_COUNT_2_BIT;

    return VK_SAMPLE_COUNT_1_BIT;
}
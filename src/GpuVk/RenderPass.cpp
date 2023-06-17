#include "RenderPass.hpp"
#include "Swapchain.hpp"

namespace GpuVk
{
RenderPass::RenderPass(std::shared_ptr<Gpu> gpu, RenderPassOptions renderPassOptions)
    : _gpu(gpu), _options(renderPassOptions)
{
    Create();
}

RenderPass::RenderPass(RenderPass&& other)
{
    *this = std::move(other);
}

RenderPass& RenderPass::operator=(RenderPass&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_options, other._options);

    std::swap(_renderPass, other._renderPass);

    std::swap(_images, other._images);
    std::swap(_framebuffers, other._framebuffers);

    std::swap(_depthImage, other._depthImage);
    std::swap(_colorImage, other._colorImage);
    std::swap(_imageFormat, other._imageFormat);
    std::swap(_msaaSampleCount, other._msaaSampleCount);

    return *this;
}

RenderPass::~RenderPass()
{
    if (!_gpu)
        return;

    CleanupResources();
    vkDestroyRenderPass(_gpu->_device, _renderPass, nullptr);
}

void RenderPass::Create()
{
    _imageFormat = _gpu->Swapchain._imageFormat;
    _msaaSampleCount = IsUsingMsaa() ? GetMaxUsableSampleCount(_gpu->_physicalDevice) : VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _imageFormat;
    colorAttachment.samples = _msaaSampleCount;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    switch (_options.ColorAttachmentUsage)
    {
        case ColorAttachmentUsage::Present:
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case ColorAttachmentUsage::PresentWithMsaa:
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case ColorAttachmentUsage::ReadFromShader:
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
    }

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat();
    depthAttachment.samples = _msaaSampleCount;
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

    std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment};

    if (IsUsingMsaa())
    {
        subpass.pResolveAttachments = &colorAttachmentResolveRef;
        attachments.push_back(colorAttachmentResolve);
    }

    if (_options.EnableDepth)
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

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_gpu->_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }

    CreateImages();
    CreateDepthResources();
    CreateColorResources();
    CreateFramebuffers();
}

void RenderPass::CreateImages()
{
    VkSwapchainKHR vkSwapchain = _gpu->Swapchain._swapchain;
    VkFormat format = _gpu->Swapchain._imageFormat;
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(_gpu->_device, vkSwapchain, &imageCount, nullptr);
    std::vector<VkImage> vkImages;
    vkImages.resize(imageCount);
    _images.clear();
    _images.reserve(imageCount);
    vkGetSwapchainImagesKHR(_gpu->_device, vkSwapchain, &imageCount, vkImages.data());

    for (VkImage vkImage : vkImages)
        _images.push_back(Image(_gpu, vkImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
}

void RenderPass::Begin(const ClearColor& clearColor)
{
    auto extent = _gpu->Swapchain._extent;
    auto currentImageIndex = _gpu->Swapchain._currentImageIndex;

    std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {{clearColor.R, clearColor.G, clearColor.B, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffers[currentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    auto commandBuffer = _gpu->Commands.GetBuffer();

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

void RenderPass::End()
{
    vkCmdEndRenderPass(_gpu->Commands.GetBuffer());
}

const bool RenderPass::IsUsingMsaa() const
{
    return _options.ColorAttachmentUsage == ColorAttachmentUsage::PresentWithMsaa;
}

const Image& RenderPass::GetColorImage() const
{
    return _colorImage;
}

void RenderPass::CreateFramebuffers()
{
    const VkExtent2D& extent = _gpu->Swapchain._extent;

    _framebuffers.resize(_images.size());

    for (size_t i = 0; i < _images.size(); i++)
    {
        std::vector<VkImageView> attachments;
        switch (_options.ColorAttachmentUsage)
        {
            case ColorAttachmentUsage::Present:
                attachments.push_back(_images[i]._view);
                attachments.push_back(_depthImage._view);
                break;
            case ColorAttachmentUsage::PresentWithMsaa:
                attachments.push_back(_colorImage._view);
                attachments.push_back(_depthImage._view);
                attachments.push_back(_images[i]._view);
                break;
            case ColorAttachmentUsage::ReadFromShader:
                attachments.push_back(_colorImage._view);
                attachments.push_back(_depthImage._view);

                break;
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_gpu->_device, &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }
}

void RenderPass::CreateDepthResources()
{
    const VkExtent2D& extent = _gpu->Swapchain._extent;

    VkFormat depthFormat = FindDepthFormat();

    _depthImage = Image(_gpu, extent.width, extent.height, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, _msaaSampleCount);
}

void RenderPass::CreateColorResources()
{
    const VkExtent2D& extent = _gpu->Swapchain._extent;

    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    switch (_options.ColorAttachmentUsage)
    {
        case ColorAttachmentUsage::Present:
        case ColorAttachmentUsage::PresentWithMsaa:
            imageUsage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            break;
        case ColorAttachmentUsage::ReadFromShader:
            imageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
    }

    _colorImage = Image(
        _gpu, extent.width, extent.height, _imageFormat, imageUsage, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, _msaaSampleCount);
}

void RenderPass::UpdateResources()
{
    CleanupResources();

    CreateImages();
    CreateDepthResources();
    CreateColorResources();
    CreateFramebuffers();
}

VkFormat RenderPass::FindSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_gpu->_physicalDevice, format, &props);

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

VkFormat RenderPass::FindDepthFormat()
{
    return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void RenderPass::CleanupResources()
{
    for (auto framebuffer : _framebuffers)
        vkDestroyFramebuffer(_gpu->_device, framebuffer, nullptr);
}

const VkSampleCountFlagBits RenderPass::GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
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
} // namespace GpuVk
#include "Image.hpp"
#include "Buffer.hpp"
#include "Gpu.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace GpuVk
{
Image::Image(std::shared_ptr<Gpu> gpu, VkImage image, VkFormat format, VkImageAspectFlags viewAspectFlags)
    : _gpu(gpu), _image(image), _format(format)
{
    CreateView(viewAspectFlags);
}

Image::Image(std::shared_ptr<Gpu> gpu, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
    VkImageAspectFlags viewAspectFlags, uint32_t mipmapLevelCount, uint32_t layerCount, VkSampleCountFlagBits samples)
    : _gpu(gpu), _format(format), _layerCount(layerCount), _width(width), _height(height),
      _mipmapLevelCount(mipmapLevelCount)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipmapLevelCount;
    imageInfo.arrayLayers = _layerCount;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;
    imageInfo.samples = samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkImage image;
    VmaAllocation allocation;

    if (vmaCreateImage(_gpu->_allocator, &imageInfo, &allocationInfo, &image, &allocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate image memory!");

    _image = image;
    _allocation = allocation;

    CreateView(viewAspectFlags);
}

Image::Image(Image&& other)
{
    *this = std::move(other);
}

Image& Image::operator=(Image&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_image, other._image);
    std::swap(_view, other._view);
    std::swap(_allocation, other._allocation);
    std::swap(_format, other._format);
    std::swap(_layerCount, other._layerCount);
    std::swap(_width, other._width);
    std::swap(_height, other._height);
    std::swap(_mipmapLevelCount, other._mipmapLevelCount);

    return *this;
}

Image::~Image()
{
    if (!_gpu)
        return;

    vkDestroyImageView(_gpu->_device, _view, nullptr);

    // Some images don't have an allocation, ie: because they were acquired
    // from the swapchain rather than allocated manually by us.
    if (!_allocation)
        return;

    vmaDestroyImage(_gpu->_allocator, _image, _allocation);
}

void Image::GenerateMipmaps()
{
    auto commandBuffer = _gpu->Commands.BeginSingleTime();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = _image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = _layerCount;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipmapWidth = _width;
    int32_t mipmapHeight = _height;

    for (uint32_t i = 1; i < _mipmapLevelCount; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipmapWidth, mipmapHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = _layerCount;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipmapWidth > 1 ? mipmapWidth / 2 : 1, mipmapHeight > 1 ? mipmapHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = _layerCount;

        vkCmdBlitImage(commandBuffer, _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &barrier);

        if (mipmapWidth > 1)
            mipmapWidth /= 2;

        if (mipmapHeight > 1)
            mipmapHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = _mipmapLevelCount - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
        nullptr, 0, nullptr, 1, &barrier);

    _gpu->Commands.EndSingleTime(commandBuffer);
}

Buffer Image::LoadImage(std::shared_ptr<Gpu> gpu, const std::string& image, int32_t& width, int32_t& height)
{
    SDL_Surface* loadedSurface = IMG_Load(image.c_str());

    if (!loadedSurface)
        throw std::runtime_error(std::string("Failed to load image: ") + image);

    SDL_Surface* surface = SDL_ConvertSurfaceFormat(loadedSurface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(loadedSurface);

    auto pixels = reinterpret_cast<uint8_t*>(surface->pixels);
    width = surface->w;
    height = surface->h;

    size_t imageSize = width * height;
    VkDeviceSize imageByteSize = imageSize * 4;

    Buffer stagingBuffer(gpu, imageByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    stagingBuffer.SetData(pixels);

    SDL_FreeSurface(surface);

    return stagingBuffer;
}

Image Image::CreateTexture(std::shared_ptr<Gpu> gpu, const std::string& image, bool enableMipmaps)
{
    int32_t texWidth, texHeight;
    Buffer stagingBuffer = LoadImage(gpu, image, texWidth, texHeight);
    uint32_t mipMapLevels = enableMipmaps ? CalculateMipmapLevelCount(texWidth, texHeight) : 1;

    Image textureImage(gpu, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, mipMapLevels);

    textureImage.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    textureImage.CopyFromBuffer(stagingBuffer);

    textureImage.GenerateMipmaps();

    return textureImage;
}

Image Image::CreateTextureArray(std::shared_ptr<Gpu> gpu, const std::string& image, bool enableMipmaps, uint32_t width,
    uint32_t height, uint32_t layers)
{
    int32_t texWidth, texHeight;
    Buffer stagingBuffer = LoadImage(gpu, image, texWidth, texHeight);
    uint32_t mipMapLevels = enableMipmaps ? CalculateMipmapLevelCount(width, height) : 1;

    Image textureImage(gpu, width, height, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, mipMapLevels, layers);

    textureImage.TransitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    textureImage.CopyFromBuffer(stagingBuffer, texWidth, texHeight);

    textureImage.GenerateMipmaps();

    return textureImage;
}

void Image::CreateView(VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image;
    viewInfo.viewType = _layerCount == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = _format;
    viewInfo.subresourceRange = {};
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = _mipmapLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = _layerCount;

    if (vkCreateImageView(_gpu->_device, &viewInfo, nullptr, &_view) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view!");
}

void Image::TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
{
    auto commandBuffer = _gpu->Commands.BeginSingleTime();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = _mipmapLevelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = _layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    _gpu->Commands.EndSingleTime(commandBuffer);
}

void Image::CopyFromBuffer(Buffer& src, uint32_t fullWidth, uint32_t fullHeight)
{
    if (fullWidth == 0)
        fullWidth = _width;

    if (fullHeight == 0)
        fullHeight = _height;

    auto commandBuffer = _gpu->Commands.BeginSingleTime();

    std::vector<VkBufferImageCopy> regions;
    uint32_t texPerRow = fullWidth / _width;

    for (uint32_t layer = 0; layer < _layerCount; layer++)
    {
        uint32_t xLayer = layer % texPerRow;
        uint32_t yLayer = layer / texPerRow;

        VkBufferImageCopy region = {};
        region.bufferOffset = (xLayer * _width + yLayer * _height * fullWidth) * 4;
        region.bufferRowLength = fullWidth;
        region.bufferImageHeight = fullHeight;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {_width, _height, 1};
        regions.push_back(region);
    }

    vkCmdCopyBufferToImage(commandBuffer, src._buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(regions.size()), regions.data());

    _gpu->Commands.EndSingleTime(commandBuffer);
}

uint32_t Image::CalculateMipmapLevelCount(int32_t texWidth, int32_t texHeight)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
}

uint32_t Image::GetWidth() const
{
    return _width;
}

uint32_t Image::GetHeight() const
{
    return _height;
}

uint32_t Image::GetMipmapLevelCount() const
{
    return _mipmapLevelCount;
}
} // namespace GpuVk
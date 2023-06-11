#pragma once

#include <cmath>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "../../deps/stb_image.h"

#include "buffer.hpp"

class Image
{
    public:
    static Image CreateTexture(const std::string &image, VmaAllocator allocator, Commands &commands,
        VkQueue graphicsQueue, VkDevice device, bool enableMipmaps);
    static Image CreateTextureArray(const std::string &image, VmaAllocator allocator, Commands &commands,
        VkQueue graphicsQueue, VkDevice device, bool enableMipmaps, uint32_t width, uint32_t height, uint32_t layers);

    Image();
    Image(VkImage image, VkFormat format);
    Image(VkImage image, VmaAllocation allocation, VkFormat format);
    Image(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, uint32_t mipmapLevels = 1, uint32_t layers = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    VkImageView CreateTextureView(VkDevice device);
    VkSampler CreateTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device,
        VkFilter minFilter = VK_FILTER_LINEAR, VkFilter magFilter = VK_FILTER_LINEAR);
    VkImageView CreateView(VkImageAspectFlags aspectFlags, VkDevice device);
    void TransitionImageLayout(
        Commands &commands, VkImageLayout oldLayout, VkImageLayout newLayout, VkQueue graphicsQueue, VkDevice device);
    void CopyFromBuffer(Buffer &src, Commands &commands, VkQueue graphicsQueue, VkDevice device, uint32_t fullWidth = 0,
        uint32_t fullHeight = 0);
    void GenerateMipmaps(Commands &commands, VkQueue graphicsQueue, VkDevice device);
    void Destroy(VmaAllocator allocator);
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    private:
    VkImage _image;
    VmaAllocation _allocation;
    VkFormat _format = VK_FORMAT_R32G32B32_SFLOAT;
    uint32_t _layerCount = 1;
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _mipmapLevels = 1;

    static Buffer LoadImage(const std::string &image, VmaAllocator allocator, int32_t &width, int32_t &height);
    static uint32_t CalcMipmapLevels(int32_t texWidth, int32_t texHeight);
};
#pragma once

#include "Commands.hpp"

#include <cmath>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class Gpu;
class Buffer;

class Image
{
    friend class RenderPass;
    friend class Pipeline;

    public:
    static Image CreateTexture(std::shared_ptr<Gpu> gpu, const std::string& image, bool enableMipmaps);
    static Image CreateTextureArray(std::shared_ptr<Gpu> gpu, const std::string& image, bool enableMipmaps,
        uint32_t width, uint32_t height, uint32_t layers);

    Image() = default;
    Image(Image&& other);
    Image& operator=(Image&& other);
    ~Image();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetMipmapLevels() const;

    private:
    Image(std::shared_ptr<Gpu> gpu, VkImage image, VkFormat format, VkImageAspectFlags viewAspectFlags);
    Image(std::shared_ptr<Gpu> gpu, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
        VkImageAspectFlags viewAspectFlags, uint32_t mipmapLevels = 1, uint32_t layerCount = 1,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

    void TransitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyFromBuffer(Buffer& src, uint32_t fullWidth = 0, uint32_t fullHeight = 0);
    void GenerateMipmaps();
    void CreateView(VkImageAspectFlags aspectFlags);


    std::shared_ptr<Gpu> _gpu;

    VkImage _image;
    VkImageView _view;
    VmaAllocation _allocation = nullptr;
    VkFormat _format = VK_FORMAT_R32G32B32_SFLOAT;
    uint32_t _layerCount = 1;
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _mipmapLevelCount = 1;

    static Buffer LoadImage(std::shared_ptr<Gpu> gpu, const std::string& image, int32_t& width, int32_t& height);
    static uint32_t CalcMipmapLevels(int32_t texWidth, int32_t texHeight);
};
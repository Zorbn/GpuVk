#include "Sampler.hpp"
#include "Gpu.hpp"

Sampler::Sampler(std::shared_ptr<Gpu> gpu, const Image& image, FilterMode minFilter, FilterMode magFilter) : _gpu(gpu)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(_gpu->PhysicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = GetVkFilter(magFilter);
    samplerInfo.minFilter = GetVkFilter(minFilter);
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.maxLod = static_cast<float>(image.GetMipmapLevels());

    if (vkCreateSampler(_gpu->Device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

Sampler::Sampler(Sampler&& other)
{
    *this = std::move(other);
}

Sampler& Sampler::operator=(Sampler&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_sampler, other._sampler);

    return *this;
}

Sampler::~Sampler()
{
    if (!_gpu)
        return;

    vkDestroySampler(_gpu->Device, _sampler, nullptr);
}
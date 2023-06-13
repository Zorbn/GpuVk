#include "Format.hpp"

VkFormat GetVkFormat(Format format)
{
    VkFormat vkFormat;

    switch (format)
    {
        case Format::Float:
            vkFormat = VK_FORMAT_R32_SFLOAT;
            break;
        case Format::Float2:
            vkFormat = VK_FORMAT_R32G32_SFLOAT;
            break;
        case Format::Float3:
            vkFormat = VK_FORMAT_R32G32B32_SFLOAT;
            break;
        case Format::Float4:
            vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
            break;
    }

    return vkFormat;
}
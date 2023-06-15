#pragma once

#include <vulkan/vulkan.h>

enum class FilterMode
{
    Linear,
    Nearest
};

VkFilter GetVkFilter(FilterMode filterMode);
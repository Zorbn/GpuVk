#include "FilterMode.hpp"

VkFilter GetVkFilter(FilterMode filterMode)
{
    VkFilter vkFilter;

    switch (filterMode)
    {
        case FilterMode::Linear:
            vkFilter = VK_FILTER_LINEAR;
            break;
        case FilterMode::Nearest:
            vkFilter = VK_FILTER_NEAREST;
            break;
    }

    return vkFilter;
}
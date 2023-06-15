#pragma once

#include <vulkan/vulkan.h>

enum class Format
{
    Float,
    Float2,
    Float3,
    Float4
};

// TODO: Use namespaces for all GpuVk files, so that names like this don't conflict with anything else.
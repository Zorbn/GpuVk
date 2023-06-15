#pragma once

#include <cinttypes>

#include "Format.hpp"

namespace GpuVk
{
struct ClearColor
{
    float R;
    float G;
    float B;
};

enum class ColorAttachmentUsage
{
    Present,
    PresentWithMsaa,
    ReadFromShader
};

struct RenderPassOptions
{
    bool EnableDepth;
    ColorAttachmentUsage ColorAttachmentUsage;
};
} // namespace GpuVk
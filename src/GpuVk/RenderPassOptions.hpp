#pragma once

#include <cinttypes>

#include "Format.hpp"

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
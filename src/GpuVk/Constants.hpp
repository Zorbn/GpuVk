#pragma once

#include <cinttypes>

namespace GpuVk
{
const uint32_t MaxFramesInFlight = 2;

#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif
} // namespace GpuVk
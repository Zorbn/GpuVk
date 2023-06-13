#pragma once

#include <cinttypes>
#include <vector>
#include <string>

#include "Format.hpp"

struct VertexAttribute
{
    uint32_t Location;
    Format Format;
    uint32_t Offset;
};

struct VertexOptions
{
    uint32_t Binding;
    uint32_t Size;
    std::vector<VertexAttribute> VertexAttributes;
};

enum class DescriptorType
{
    UniformBuffer,
    ImageSampler
};

enum class ShaderStage
{
    Vertex,
    Fragment
};

struct DescriptorLayout
{
    uint32_t Binding;
    DescriptorType Type;
    ShaderStage ShaderStage;
};

struct PipelineOptions
{
    std::string VertexShader;
    std::string FragmentShader;
    bool EnableTransparency;
    VertexOptions VertexDataOptions;
    VertexOptions InstanceDataOptions;
    std::vector<DescriptorLayout> DescriptorLayouts;
};
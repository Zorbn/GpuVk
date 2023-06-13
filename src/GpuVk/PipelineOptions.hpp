#pragma once

#include <cinttypes>
#include <vector>
#include <string>

enum class VertexAttributeFormat
{
    Float,
    Float2,
    Float3
};

struct VertexAttribute
{
    uint32_t Location;
    VertexAttributeFormat Format;
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
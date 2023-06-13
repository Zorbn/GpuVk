#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

#include "Gpu.hpp"
#include "RenderPass.hpp"
#include "Swapchain.hpp"
#include "Constants.hpp"
#include "UniformBuffer.hpp"
#include "PipelineOptions.hpp"

class Pipeline
{
    public:
    Pipeline() = default;
    Pipeline(std::shared_ptr<Gpu> gpu, const PipelineOptions& pipelineOptions, const RenderPass& renderPass);
    Pipeline(const Pipeline& other) = delete;
    Pipeline& operator=(const Pipeline& other) = delete;
    Pipeline(Pipeline&& other);
    Pipeline& operator=(Pipeline&& other);
    ~Pipeline();

    template <typename T> void UpdateUniform(uint32_t binding, const UniformBuffer<T>& uniformBuffer)
    {
        for (uint32_t i = 0; i < MaxFramesInFlight; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffer.GetBuffer(i);
            bufferInfo.offset = 0;
            bufferInfo.range = uniformBuffer.GetDataSize();

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = _descriptorSets[i];
            descriptorWrite.dstBinding = binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(_gpu->Device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void UpdateImage(uint32_t binding, VkImageView imageView, VkSampler imageSampler);

    void Bind();

    private:
    static VkDescriptorType GetVkDescriptorType(DescriptorType descriptorType);
    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    std::array<VkVertexInputBindingDescription, 2> CreateVertexInputBindingDescriptions(const PipelineOptions& pipelineOptions);
    std::vector<VkVertexInputAttributeDescription> CreateVertexInputAttributeDescriptions(const VertexOptions& vertexOptions);
    void Create(const PipelineOptions& pipelineOptions, const RenderPass& renderPass);

    static VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device);
    static std::vector<char> ReadFile(const std::string& filename); // TODO: Move this somewhere else.

    std::shared_ptr<Gpu> _gpu;

    VkPipelineLayout _pipelineLayout;
    VkPipeline _pipeline;

    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<DescriptorLayout> _descriptorLayouts;

    bool _enableTransparency = false;
};
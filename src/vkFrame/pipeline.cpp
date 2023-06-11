#include "pipeline.hpp"

void Pipeline::CreateDescriptorSetLayout(
    VkDevice device, std::function<void(std::vector<VkDescriptorSetLayoutBinding> &)> setupBindings)
{
    _setupBindings = setupBindings;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    _setupBindings(bindings);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void Pipeline::CreateDescriptorPool(const uint32_t maxFramesInFlight, VkDevice device,
    std::function<void(std::vector<VkDescriptorPoolSize> &poolSizes)> setupPool)
{
    _setupPool = setupPool;

    std::vector<VkDescriptorPoolSize> poolSizes;
    _setupPool(poolSizes);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(maxFramesInFlight);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void Pipeline::CreateDescriptorSets(const uint32_t maxFramesInFlight, VkDevice device,
    std::function<void(std::vector<VkWriteDescriptorSet> &, VkDescriptorSet, uint32_t)> setupDescriptor)
{
    _setupDescriptor = setupDescriptor;

    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, _descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(maxFramesInFlight);
    if (vkAllocateDescriptorSets(device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        _setupDescriptor(descriptorWrites, _descriptorSets[i], i);
    }
}

void Pipeline::Bind(VkCommandBuffer commandBuffer, int32_t currentFrame)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1,
        &_descriptorSets[currentFrame], 0, nullptr);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
}

VkShaderModule Pipeline::CreateShaderModule(const std::vector<char> &code, VkDevice device)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> Pipeline::ReadFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void Pipeline::Cleanup(VkDevice device)
{
    vkDestroyPipeline(device, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, _descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, _descriptorSetLayout, nullptr);
}
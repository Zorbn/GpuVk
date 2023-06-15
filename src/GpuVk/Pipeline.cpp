#include "Pipeline.hpp"
#include "File.hpp"

Pipeline::Pipeline(std::shared_ptr<Gpu> gpu, const PipelineOptions& pipelineOptions, const RenderPass& renderPass) : _gpu(gpu)
{
    Create(pipelineOptions, renderPass);
}

Pipeline::Pipeline(Pipeline&& other)
{
    *this = std::move(other);
}

Pipeline& Pipeline::operator=(Pipeline&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_pipelineLayout, other._pipelineLayout);
    std::swap(_pipeline, other._pipeline);

    std::swap(_descriptorSetLayout, other._descriptorSetLayout);
    std::swap(_descriptorPool, other._descriptorPool);
    std::swap(_descriptorSets, other._descriptorSets);
    std::swap(_descriptorLayouts, other._descriptorLayouts);

    std::swap(_enableTransparency, other._enableTransparency);

    return *this;
}

Pipeline::~Pipeline()
{
    if (!_gpu)
        return;

    vkDestroyPipeline(_gpu->_device, _pipeline, nullptr);
    vkDestroyPipelineLayout(_gpu->_device, _pipelineLayout, nullptr);
    vkDestroyDescriptorPool(_gpu->_device, _descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(_gpu->_device, _descriptorSetLayout, nullptr);
}

void Pipeline::UpdateImage(uint32_t binding, const Image& image, const Sampler& sampler)
{
    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = image._view;
        imageInfo.sampler = sampler._sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = _descriptorSets[i];
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_gpu->_device, 1, &descriptorWrite, 0, nullptr);
    }
}

VkDescriptorType Pipeline::GetVkDescriptorType(DescriptorType descriptorType)
{
    VkDescriptorType vkDescriptorType;

    switch (descriptorType)
    {
        case DescriptorType::UniformBuffer:
            vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case DescriptorType::ImageSampler:
            vkDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
    }

    return vkDescriptorType;
}

void Pipeline::CreateDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(_descriptorLayouts.size());
    for (auto layout : _descriptorLayouts)
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = layout.Binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = GetVkDescriptorType(layout.Type);
        layoutBinding.pImmutableSamplers = nullptr;

        switch (layout.ShaderStage)
        {
            case ShaderStage::Vertex:
                layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderStage::Fragment:
                layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
        }

        bindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(_gpu->_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void Pipeline::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(_descriptorLayouts.size());
    for (auto layout : _descriptorLayouts)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = GetVkDescriptorType(layout.Type);
        poolSize.descriptorCount = MaxFramesInFlight;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MaxFramesInFlight);

    if (vkCreateDescriptorPool(_gpu->_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void Pipeline::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MaxFramesInFlight, _descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MaxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(MaxFramesInFlight);
    if (vkAllocateDescriptorSets(_gpu->_device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }
}

std::array<VkVertexInputBindingDescription, 2> Pipeline::CreateVertexInputBindingDescriptions(
    const PipelineOptions& pipelineOptions)
{
    std::array<VkVertexInputBindingDescription, 2> bindingDescriptions;

    bindingDescriptions[0].binding = pipelineOptions.VertexDataOptions.Binding;
    bindingDescriptions[0].stride = pipelineOptions.VertexDataOptions.Size;
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescriptions[1].binding = pipelineOptions.InstanceDataOptions.Binding;
    bindingDescriptions[1].stride = pipelineOptions.InstanceDataOptions.Size;
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Pipeline::CreateVertexInputAttributeDescriptions(
    const VertexOptions& vertexOptions)
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.reserve(vertexOptions.VertexAttributes.size());

    for (auto attribute : vertexOptions.VertexAttributes)
    {
        VkVertexInputAttributeDescription attributeDescription{};
        attributeDescription.binding = vertexOptions.Binding;
        attributeDescription.location = attribute.Location;
        attributeDescription.format = GetVkFormat(attribute.Format);
        attributeDescription.offset = attribute.Offset;
        attributeDescriptions.push_back(attributeDescription);
    }

    return attributeDescriptions;
}

void Pipeline::Create(const PipelineOptions& pipelineOptions, const RenderPass& renderPass)
{
    _descriptorLayouts = pipelineOptions.DescriptorLayouts;
    _enableTransparency = pipelineOptions.EnableTransparency;

    CreateDescriptorSetLayout();
    CreateDescriptorPool();
    CreateDescriptorSets();

    auto vertShaderCode = ReadFile(pipelineOptions.VertexShader);
    auto fragShaderCode = ReadFile(pipelineOptions.FragmentShader);

    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode, _gpu->_device);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode, _gpu->_device);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescriptions = CreateVertexInputBindingDescriptions(pipelineOptions);
    auto vertexAttributeDescriptions = CreateVertexInputAttributeDescriptions(pipelineOptions.VertexDataOptions);
    auto instanceAttributeDescriptions = CreateVertexInputAttributeDescriptions(pipelineOptions.InstanceDataOptions);
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.reserve(vertexAttributeDescriptions.size() + instanceAttributeDescriptions.size());

    for (VkVertexInputAttributeDescription desc : vertexAttributeDescriptions)
    {
        attributeDescriptions.push_back(desc);
    }

    for (VkVertexInputAttributeDescription desc : instanceAttributeDescriptions)
    {
        attributeDescriptions.push_back(desc);
    }

    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    if (renderPass.IsUsingMsaa())
    {
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.minSampleShading = 0.2f;
        multisampling.rasterizationSamples = renderPass._msaaSampleCount;
    }
    else
    {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    if (_enableTransparency)
    {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    else
    {
        colorBlendAttachment.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;

    if (vkCreatePipelineLayout(_gpu->_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = renderPass._renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(_gpu->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(_gpu->_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(_gpu->_device, vertShaderModule, nullptr);
}

void Pipeline::Bind()
{
    auto currentBufferIndex = _gpu->Commands._currentBufferIndex;
    vkCmdBindDescriptorSets(_gpu->Commands.GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1,
        &_descriptorSets[currentBufferIndex], 0, nullptr);
    vkCmdBindPipeline(_gpu->Commands.GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

VkShaderModule Pipeline::CreateShaderModule(const std::vector<char>& code, VkDevice device)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

VkFormat Pipeline::GetVkFormat(Format format)
{
    switch (format)
    {
        case Format::Float:
            return VK_FORMAT_R32_SFLOAT;
        case Format::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case Format::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            throw std::runtime_error("Tried to get a VkFormat from an invalid format!");
    }
}
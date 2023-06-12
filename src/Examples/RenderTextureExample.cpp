#include "../GpuVk/RenderEngine.hpp"

/*
 * RenderTexture:
 * Uses multiple passes to draw to a model onto itself.
 */

struct VertexData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 TexCoord;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexData);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexData, Pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexData, Color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexData, TexCoord);

        return attributeDescriptions;
    }
};

struct InstanceData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 1;
        bindingDescription.stride = sizeof(InstanceData);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.resize(1);

        attributeDescriptions[0].binding = 1;
        attributeDescriptions[0].location = 3;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        return attributeDescriptions;
    }
};

struct UniformBufferData
{
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};

const int32_t MapSize = 4;

const std::array<int32_t, MapSize* MapSize* MapSize> VoxelData = {
    1, 0, 0, 0, 0, 4, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2, // 1
    0, 0, 0, 1, 0, 0, 3, 0, 0, 4, 0, 0, 2, 0, 0, 0, // 2
    3, 2, 1, 4, 2, 0, 0, 1, 1, 0, 0, 2, 4, 1, 2, 3, // 3
    0, 0, 0, 0, 0, 1, 2, 0, 0, 4, 3, 0, 0, 0, 0, 0, // 4
};

const std::array<std::array<glm::vec3, 4>, 6> CubeVertices = {{
    // Forward
    {
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(1, 1, 0),
        glm::vec3(1, 0, 0),
    },
    // Backward
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 0, 1),
    },
    // Right
    {
        glm::vec3(1, 0, 0),
        glm::vec3(1, 0, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, 0),
    },
    // Left
    {
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 1),
        glm::vec3(0, 1, 0),
    },
    // Up
    {
        glm::vec3(0, 1, 0),
        glm::vec3(0, 1, 1),
        glm::vec3(1, 1, 1),
        glm::vec3(1, 1, 0),
    },
    // Down
    {
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(1, 0, 1),
        glm::vec3(1, 0, 0),
    },
}};

const std::array<std::array<glm::vec2, 4>, 6> CubeUvs = {{
    // Forward
    {
        glm::vec2(1, 1),
        glm::vec2(1, 0),
        glm::vec2(0, 0),
        glm::vec2(0, 1),
    },
    // Backward
    {
        glm::vec2(0, 1),
        glm::vec2(0, 0),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
    },
    // Right
    {
        glm::vec2(1, 1),
        glm::vec2(0, 1),
        glm::vec2(0, 0),
        glm::vec2(1, 0),
    },
    // Left
    {
        glm::vec2(0, 1),
        glm::vec2(1, 1),
        glm::vec2(1, 0),
        glm::vec2(0, 0),
    },
    // Up
    {
        glm::vec2(0, 1),
        glm::vec2(0, 0),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
    },
    // Down
    {
        glm::vec2(0, 1),
        glm::vec2(0, 0),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
    },
}};

const std::array<std::array<uint16_t, 6>, 6> CubeIndices = {{
    {0, 1, 2, 0, 2, 3}, // Forward
    {0, 2, 1, 0, 3, 2}, // Backward
    {0, 2, 1, 0, 3, 2}, // Right
    {0, 1, 2, 0, 2, 3}, // Left
    {0, 1, 2, 0, 2, 3}, // Up
    {0, 2, 1, 0, 3, 2}, // Down
}};

const std::array<std::array<int32_t, 3>, 6> Directions = {{
    {0, 0, -1}, // Forward
    {0, 0, 1},  // Backward
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Up
    {0, -1, 0}, // Down
}};

class App : public IRenderer
{
    private:
    Pipeline _pipeline;
    Pipeline _finalPipeline;
    RenderPass _renderPass;
    RenderPass _finalRenderPass;

    Image _textureImage;
    VkImageView _textureImageView;
    VkSampler _textureSampler;

    Image _colorImage;
    VkImageView _colorImageView;
    VkSampler _colorSampler;

    Image _depthImage;
    VkImageView _depthImageView;

    UniformBuffer<UniformBufferData> _ubo;
    Model<VertexData, uint16_t, InstanceData> _voxelModel;

    std::vector<VertexData> _voxelVertices;
    std::vector<uint16_t> _voxelIndices;
    std::vector<VkClearValue> _clearValues;

    public:
    int32_t GetVoxel(size_t x, size_t y, size_t z)
    {
        if (x < 0 || x >= MapSize || y < 0 || y >= MapSize || z < 0 || z >= MapSize)
        {
            return 0;
        }

        return VoxelData[x + y * MapSize + z * MapSize * MapSize];
    }

    void GenerateVoxelMesh()
    {
        for (size_t x = 0; x < MapSize; x++)
            for (size_t y = 0; y < MapSize; y++)
                for (size_t z = 0; z < MapSize; z++)
                {
                    int32_t voxel = GetVoxel(x, y, z);

                    if (voxel == 0)
                        continue;

                    for (size_t face = 0; face < 6; face++)
                    {
                        if (GetVoxel(x + +Directions[face][0], y + Directions[face][1], z + Directions[face][2]) != 0)
                            continue;

                        size_t vertexCount = _voxelVertices.size();
                        for (uint16_t index : CubeIndices[face])
                        {
                            _voxelIndices.push_back(index + vertexCount);
                        }

                        for (size_t i = 0; i < 4; i++)
                        {
                            glm::vec3 vertex = CubeVertices[face][i];
                            glm::vec2 uv = CubeUvs[face][i];

                            _voxelVertices.push_back(VertexData{
                                vertex + glm::vec3(x, y, z),
                                glm::vec3(1.0, 1.0, 1.0),
                                glm::vec3(uv.x, uv.y, voxel - 1),
                            });
                        }
                    }
                }
    }

    void Init(VulkanState& vulkanState, SDL_Window* window, int32_t width, int32_t height)
    {
        (void)window;

        vulkanState.Swapchain.Create(
            vulkanState.Device, vulkanState.PhysicalDevice, vulkanState.Surface, width, height);

        vulkanState.Commands.CreatePool(
            vulkanState.PhysicalDevice, vulkanState.Device, vulkanState.Surface);
        vulkanState.Commands.CreateBuffers(vulkanState.Device);

        _textureImage = Image::CreateTextureArray("res/cubesImg.png", vulkanState.Allocator, vulkanState.Commands,
            vulkanState.GraphicsQueue, vulkanState.Device, true, 16, 16, 4);
        _textureImageView = _textureImage.CreateTextureView(vulkanState.Device);
        _textureSampler = _textureImage.CreateTextureSampler(
            vulkanState.PhysicalDevice, vulkanState.Device, VK_FILTER_NEAREST, VK_FILTER_NEAREST);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = 1.0f;

        if (vkCreateSampler(vulkanState.Device, &samplerInfo, nullptr, &_colorSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create color sampler!");
        }

        GenerateVoxelMesh();
        _voxelModel = Model<VertexData, uint16_t, InstanceData>::FromVerticesAndIndices(_voxelVertices, _voxelIndices,
            2, vulkanState.Allocator, vulkanState.Commands, vulkanState.GraphicsQueue,
            vulkanState.Device);
        std::vector<InstanceData> instances = {
            InstanceData{glm::vec3(0.0, 0.0, 0.0)}, InstanceData{glm::vec3(-2.0, 0.0, -5.0)}};
        _voxelModel.UpdateInstances(instances, vulkanState.Commands, vulkanState.Allocator,
            vulkanState.GraphicsQueue, vulkanState.Device);

        const VkExtent2D& extent = vulkanState.Swapchain.GetExtent();
        _ubo.Create(vulkanState.Allocator);

        _renderPass.CreateCustom(
            vulkanState.Device, vulkanState.Swapchain,
            [&]
            {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkFormat depthFormat = _renderPass.FindDepthFormat(vulkanState.PhysicalDevice);
                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = depthFormat;
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 1;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;
                subpass.pDepthStencilAttachment = &depthAttachmentRef;

                std::array<VkSubpassDependency, 2> dependencies;

                dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass = 0;
                dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[0].dstAccessMask =
                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[1].srcSubpass = 0;
                dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                dependencies[1].srcAccessMask =
                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                renderPassInfo.pAttachments = attachments.data();
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpass;
                renderPassInfo.dependencyCount = dependencies.size();
                renderPassInfo.pDependencies = dependencies.data();

                VkRenderPass renderPass;

                if (vkCreateRenderPass(vulkanState.Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create render pass!");
                }

                return renderPass;
            },
            [&](const VkExtent2D& extent)
            {
                _colorImage =
                    Image(vulkanState.Allocator, extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT,
                        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                _colorImageView = _colorImage.CreateView(VK_IMAGE_ASPECT_COLOR_BIT, vulkanState.Device);

                VkFormat depthFormat = _renderPass.FindDepthFormat(vulkanState.PhysicalDevice);
                _depthImage = Image(vulkanState.Allocator, extent.width, extent.height, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                _depthImageView = _depthImage.CreateView(VK_IMAGE_ASPECT_DEPTH_BIT, vulkanState.Device);
            },
            [=]
            {
                vkDestroyImageView(vulkanState.Device, _colorImageView, nullptr);
                _colorImage.Destroy(vulkanState.Allocator);

                vkDestroyImageView(vulkanState.Device, _depthImageView, nullptr);
                _depthImage.Destroy(vulkanState.Allocator);
            },
            [&](std::vector<VkImageView>& attachments, VkImageView imageView)
            {
                attachments.push_back(_colorImageView);
                attachments.push_back(_depthImageView);
            });

        _finalRenderPass.Create(vulkanState.PhysicalDevice, vulkanState.Device, vulkanState.Allocator,
            vulkanState.Swapchain, true, false);

        _finalPipeline.CreateDescriptorSetLayout(vulkanState.Device,
            [&](std::vector<VkDescriptorSetLayoutBinding>& bindings)
            {
                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.pImmutableSamplers = nullptr;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 1;
                samplerLayoutBinding.descriptorCount = 1;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.pImmutableSamplers = nullptr;
                samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                VkDescriptorSetLayoutBinding depthSamplerLayoutBinding{};
                depthSamplerLayoutBinding.binding = 2;
                depthSamplerLayoutBinding.descriptorCount = 1;
                depthSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                depthSamplerLayoutBinding.pImmutableSamplers = nullptr;
                depthSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(uboLayoutBinding);
                bindings.push_back(samplerLayoutBinding);
                bindings.push_back(depthSamplerLayoutBinding);
            });
        _finalPipeline.CreateDescriptorPool(vulkanState.Device,
            [&](std::vector<VkDescriptorPoolSize> poolSizes)
            {
                poolSizes.resize(3);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(MaxFramesInFlight);
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(MaxFramesInFlight);
                poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[2].descriptorCount = static_cast<uint32_t>(MaxFramesInFlight);
            });
        _finalPipeline.CreateDescriptorSets(vulkanState.Device,
            [&](std::vector<VkWriteDescriptorSet>& descriptorWrites, VkDescriptorSet descriptorSet, uint32_t i)
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = _ubo.GetBuffer(i);
                bufferInfo.offset = 0;
                bufferInfo.range = _ubo.GetDataSize();

                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = _textureImageView;
                imageInfo.sampler = _textureSampler;

                VkDescriptorImageInfo depthImageInfo{};
                depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                depthImageInfo.imageView = _colorImageView;
                depthImageInfo.sampler = _colorSampler;

                descriptorWrites.resize(3);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSet;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &bufferInfo;

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstSet = descriptorSet;
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pImageInfo = &imageInfo;

                descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[2].dstSet = descriptorSet;
                descriptorWrites[2].dstBinding = 2;
                descriptorWrites[2].dstArrayElement = 0;
                descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[2].descriptorCount = 1;
                descriptorWrites[2].pImageInfo = &depthImageInfo;

                vkUpdateDescriptorSets(vulkanState.Device, static_cast<uint32_t>(descriptorWrites.size()),
                    descriptorWrites.data(), 0, nullptr);
            });
        _finalPipeline.Create<VertexData, InstanceData>("res/renderTextureFinalShader.vert.spv",
            "res/renderTextureFinalShader.frag.spv", vulkanState.Device, _finalRenderPass, false);

        _pipeline.CreateDescriptorSetLayout(vulkanState.Device,
            [&](std::vector<VkDescriptorSetLayoutBinding>& bindings)
            {
                VkDescriptorSetLayoutBinding uboLayoutBinding{};
                uboLayoutBinding.binding = 0;
                uboLayoutBinding.descriptorCount = 1;
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uboLayoutBinding.pImmutableSamplers = nullptr;
                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(uboLayoutBinding);
            });
        _pipeline.CreateDescriptorPool(vulkanState.Device,
            [&](std::vector<VkDescriptorPoolSize> poolSizes)
            {
                poolSizes.resize(1);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(MaxFramesInFlight);
            });
        _pipeline.CreateDescriptorSets(vulkanState.Device,
            [&](std::vector<VkWriteDescriptorSet>& descriptorWrites, VkDescriptorSet descriptorSet, size_t i)
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = _ubo.GetBuffer(i);
                bufferInfo.offset = 0;
                bufferInfo.range = _ubo.GetDataSize();

                descriptorWrites.resize(1);

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSet;
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &bufferInfo;

                vkUpdateDescriptorSets(vulkanState.Device, static_cast<uint32_t>(descriptorWrites.size()),
                    descriptorWrites.data(), 0, nullptr);
            });
        _pipeline.Create<VertexData, InstanceData>("res/renderTextureShader.vert.spv",
            "res/renderTextureShader.frag.spv", vulkanState.Device, _renderPass, false);

        _clearValues.resize(2);
        _clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        _clearValues[1].depthStencil = {1.0f, 0};
    }

    void Update(VulkanState& vulkanState)
    {
    }

    void Render(VulkanState& vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
    {
        const VkExtent2D& extent = vulkanState.Swapchain.GetExtent();

        UniformBufferData uboData{};
        uboData.Model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.View =
            glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.Proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 20.0f);
        uboData.Proj[1][1] *= -1;

        _ubo.Update(uboData);

        vulkanState.Commands.BeginBuffer(currentFrame);

        _clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        _renderPass.Begin(imageIndex, commandBuffer, extent, _clearValues);
        _pipeline.Bind(commandBuffer, currentFrame);

        _voxelModel.Draw(commandBuffer);

        _renderPass.End(commandBuffer);

        _clearValues[0].color = {{0.0f, 0.0f, 1.0f, 1.0f}};
        _finalRenderPass.Begin(imageIndex, commandBuffer, extent, _clearValues);
        _finalPipeline.Bind(commandBuffer, currentFrame);

        _voxelModel.Draw(commandBuffer);

        _finalRenderPass.End(commandBuffer);

        vulkanState.Commands.EndBuffer(currentFrame);
    }

    void Resize(VulkanState& vulkanState, int32_t width, int32_t height)
    {
        (void)width, (void)height;

        _renderPass.Recreate(vulkanState.Device, vulkanState.Swapchain);
        _finalRenderPass.Recreate(vulkanState.Device, vulkanState.Swapchain);
        _finalPipeline.Recreate<VertexData, InstanceData>(vulkanState.Device, _finalRenderPass);
    }

    void Cleanup(VulkanState& vulkanState)
    {
        _pipeline.Cleanup(vulkanState.Device);
        _finalPipeline.Cleanup(vulkanState.Device);
        _renderPass.Cleanup(vulkanState.Device);
        _finalRenderPass.Cleanup(vulkanState.Device);

        _ubo.Destroy(vulkanState.Allocator);

        vkDestroySampler(vulkanState.Device, _colorSampler, nullptr);

        vkDestroySampler(vulkanState.Device, _textureSampler, nullptr);
        vkDestroyImageView(vulkanState.Device, _textureImageView, nullptr);
        _textureImage.Destroy(vulkanState.Allocator);

        _voxelModel.Destroy(vulkanState.Allocator);
    }
};

int main()
{
    try
    {
        RenderEngine renderEngine;
        App app;
        renderEngine.Run("Render Texture", 640, 480, app);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
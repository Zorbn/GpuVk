#include "../GpuVk/RenderEngine.hpp"

/*
 * Cubes:
 * Generate a small voxel mesh. The cubes were a lie, there aren't really any cubes.
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
    RenderPass _renderPass;

    Image _textureImage;
    VkImageView _textureImageView;
    VkSampler _textureSampler;

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
                        if (GetVoxel(x + Directions[face][0], y + Directions[face][1], z + Directions[face][2]) != 0)
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

    void Init(Gpu& gpu, SDL_Window* window, int32_t width, int32_t height)
    {
        (void)window;

        gpu.Swapchain.Create(
            gpu.Device, gpu.PhysicalDevice, gpu.Surface, width, height);

        gpu.Commands.CreatePool(
            gpu.PhysicalDevice, gpu.Device, gpu.Surface);
        gpu.Commands.CreateBuffers(gpu.Device);

        _textureImage = Image::CreateTextureArray("res/cubesImg.png", gpu.Allocator, gpu.Commands,
            gpu.GraphicsQueue, gpu.Device, true, 16, 16, 4);
        _textureImageView = _textureImage.CreateTextureView(gpu.Device);
        _textureSampler = _textureImage.CreateTextureSampler(
            gpu.PhysicalDevice, gpu.Device, VK_FILTER_NEAREST, VK_FILTER_NEAREST);

        GenerateVoxelMesh();
        _voxelModel = Model<VertexData, uint16_t, InstanceData>::FromVerticesAndIndices(_voxelVertices, _voxelIndices,
            1, gpu.Allocator, gpu.Commands, gpu.GraphicsQueue,
            gpu.Device);
        std::vector<InstanceData> instances = {InstanceData{}};
        _voxelModel.UpdateInstances(instances, gpu.Commands, gpu.Allocator,
            gpu.GraphicsQueue, gpu.Device);

        const VkExtent2D& extent = gpu.Swapchain.GetExtent();
        _ubo.Create(gpu.Allocator);

        _renderPass.Create(gpu.PhysicalDevice, gpu.Device, gpu.Allocator,
            gpu.Swapchain, true, false);

        _pipeline.CreateDescriptorSetLayout(gpu.Device,
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

                bindings.push_back(uboLayoutBinding);
                bindings.push_back(samplerLayoutBinding);
            });
        _pipeline.CreateDescriptorPool(gpu.Device,
            [&](std::vector<VkDescriptorPoolSize> poolSizes)
            {
                poolSizes.resize(2);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                // TODO: Make MaxFramesInFlight unnecessary for the user to know.
                poolSizes[0].descriptorCount = static_cast<uint32_t>(MaxFramesInFlight);
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(MaxFramesInFlight);
            });
        _pipeline.CreateDescriptorSets(gpu.Device,
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

                descriptorWrites.resize(2);

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

                vkUpdateDescriptorSets(gpu.Device, static_cast<uint32_t>(descriptorWrites.size()),
                    descriptorWrites.data(), 0, nullptr);
            });
        _pipeline.Create<VertexData, InstanceData>(
            "res/cubesShader.vert.spv", "res/cubesShader.frag.spv", gpu.Device, _renderPass, false);

        _clearValues.resize(2);
        _clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        _clearValues[1].depthStencil = {1.0f, 0};
    }

    void Update(Gpu& gpu)
    {
    }

    void Render(Gpu& gpu)
    {
        const VkExtent2D& extent = gpu.Swapchain.GetExtent();

        UniformBufferData uboData{};
        uboData.Model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.View =
            glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.Proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 20.0f);
        uboData.Proj[1][1] *= -1;

        _ubo.Update(uboData);

        gpu.Commands.BeginBuffer();

        _renderPass.Begin(gpu.Swapchain, gpu.Commands, extent, _clearValues);
        _pipeline.Bind(gpu.Commands);

        _voxelModel.Draw(gpu.Commands);

        _renderPass.End(gpu.Commands);

        gpu.Commands.EndBuffer();
    }

    void Resize(Gpu& gpu, int32_t width, int32_t height)
    {
        (void)width, (void)height;

        _renderPass.Recreate(gpu.Device, gpu.Swapchain);
    }

    void Cleanup(Gpu& gpu)
    {
        _pipeline.Cleanup(gpu.Device);
        _renderPass.Cleanup(gpu.Device);

        _ubo.Destroy(gpu.Allocator);

        vkDestroySampler(gpu.Device, _textureSampler, nullptr);
        vkDestroyImageView(gpu.Device, _textureImageView, nullptr);
        _textureImage.Destroy(gpu.Allocator);

        _voxelModel.Destroy(gpu.Allocator);
    }
};

int main()
{
    try
    {
        RenderEngine renderEngine;
        App app;
        renderEngine.Run("Cubes", 640, 480, app);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
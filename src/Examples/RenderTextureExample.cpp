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
};

struct InstanceData
{
    glm::vec3 Pos;
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
    VkSampler _textureSampler;

    VkSampler _colorSampler;

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

    void Init(std::shared_ptr<Gpu> gpu, SDL_Window* window, int32_t width, int32_t height)
    {
        (void)window;

        _textureImage = Image::CreateTextureArray(gpu, "res/cubesImg.png", true, 16, 16, 4);
        _textureSampler = _textureImage.CreateTextureSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST);

        GenerateVoxelMesh();
        _voxelModel =
            Model<VertexData, uint16_t, InstanceData>::FromVerticesAndIndices(gpu, _voxelVertices, _voxelIndices, 2);
        std::vector<InstanceData> instances = {
            InstanceData{glm::vec3(0.0, 0.0, 0.0)}, InstanceData{glm::vec3(-2.0, 0.0, -5.0)}};
        _voxelModel.UpdateInstances(instances);

        const VkExtent2D& extent = gpu->Swapchain.GetExtent();
        _ubo = UniformBuffer<UniformBufferData>(gpu);

        RenderPassOptions renderPassOptions{};
        renderPassOptions.EnableDepth = true;
        renderPassOptions.ColorAttachmentUsage = ColorAttachmentUsage::ReadFromShader;
        _renderPass = RenderPass(gpu, renderPassOptions);
        _colorSampler = _renderPass.GetColorImage().CreateTextureSampler();

        RenderPassOptions finalRenderPassOptions{};
        finalRenderPassOptions.EnableDepth = true;
        finalRenderPassOptions.ColorAttachmentUsage = ColorAttachmentUsage::PresentWithMsaa;
        _finalRenderPass = RenderPass(gpu, finalRenderPassOptions);

        VertexOptions vertexDataOptions{};
        vertexDataOptions.Binding = 0;
        vertexDataOptions.Size = sizeof(VertexData);
        vertexDataOptions.VertexAttributes.push_back({
            .Location = 0,
            .Format = Format::Float3,
            .Offset = static_cast<uint32_t>(offsetof(VertexData, VertexData::Pos)),
        });
        vertexDataOptions.VertexAttributes.push_back({
            .Location = 1,
            .Format = Format::Float3,
            .Offset = static_cast<uint32_t>(offsetof(VertexData, VertexData::Color)),
        });
        vertexDataOptions.VertexAttributes.push_back({
            .Location = 2,
            .Format = Format::Float3,
            .Offset = static_cast<uint32_t>(offsetof(VertexData, VertexData::TexCoord)),
        });
        VertexOptions instanceDataOptions{};
        instanceDataOptions.Binding = 1;
        instanceDataOptions.Size = sizeof(InstanceData);
        instanceDataOptions.VertexAttributes.push_back({
            .Location = 3,
            .Format = Format::Float3,
            .Offset = 0,
        });

        PipelineOptions finalPipelineOptions{};
        finalPipelineOptions.VertexShader = "res/renderTextureFinalShader.vert.spv";
        finalPipelineOptions.FragmentShader = "res/renderTextureFinalShader.frag.spv";
        finalPipelineOptions.EnableTransparency = false;
        finalPipelineOptions.VertexDataOptions = vertexDataOptions;
        finalPipelineOptions.InstanceDataOptions = instanceDataOptions;
        finalPipelineOptions.DescriptorLayouts.push_back({
            .Binding = 0,
            .Type = DescriptorType::UniformBuffer,
            .ShaderStage = ShaderStage::Vertex,
        });
        finalPipelineOptions.DescriptorLayouts.push_back({
            .Binding = 1,
            .Type = DescriptorType::ImageSampler,
            .ShaderStage = ShaderStage::Fragment,
        });
        finalPipelineOptions.DescriptorLayouts.push_back({
            .Binding = 2,
            .Type = DescriptorType::ImageSampler,
            .ShaderStage = ShaderStage::Fragment,
        });
        _finalPipeline = Pipeline(gpu, finalPipelineOptions, _finalRenderPass);
        _finalPipeline.UpdateUniform(0, _ubo);
        // TODO: Update image should take an image rather than a raw image view:
        _finalPipeline.UpdateImage(1, _textureImage.GetView(), _textureSampler);
        _finalPipeline.UpdateImage(2, _renderPass.GetColorImage().GetView(), _colorSampler);

        PipelineOptions pipelineOptions{};
        pipelineOptions.VertexShader = "res/renderTextureShader.vert.spv";
        pipelineOptions.FragmentShader = "res/renderTextureShader.frag.spv";
        pipelineOptions.EnableTransparency = false;
        pipelineOptions.VertexDataOptions = vertexDataOptions;
        pipelineOptions.InstanceDataOptions = instanceDataOptions;
        pipelineOptions.DescriptorLayouts.push_back({
            .Binding = 0,
            .Type = DescriptorType::UniformBuffer,
            .ShaderStage = ShaderStage::Vertex,
        });
        _pipeline = Pipeline(gpu, pipelineOptions, _renderPass);
        _pipeline.UpdateUniform(0, _ubo);

        _clearValues.resize(2);
        _clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        _clearValues[1].depthStencil = {1.0f, 0};
    }

    void Update(std::shared_ptr<Gpu> gpu)
    {
    }

    void Render(std::shared_ptr<Gpu> gpu)
    {
        // TODO: This is not a good way to handle getting the window width/height.
        auto extent = gpu->Swapchain.GetExtent();

        UniformBufferData uboData{};
        uboData.Model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.View =
            glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.Proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 20.0f);
        uboData.Proj[1][1] *= -1;

        _ubo.Update(uboData);

        gpu->Commands.BeginBuffer();

        _clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        _renderPass.Begin(_clearValues);
        _pipeline.Bind();

        _voxelModel.Draw();

        _renderPass.End();

        _clearValues[0].color = {{0.0f, 0.0f, 1.0f, 1.0f}};
        _finalRenderPass.Begin(_clearValues);
        _finalPipeline.Bind();

        _voxelModel.Draw();

        _finalRenderPass.End();

        gpu->Commands.EndBuffer();
    }

    void Resize(std::shared_ptr<Gpu> gpu, int32_t width, int32_t height)
    {
        (void)width, (void)height;

        _renderPass.UpdateResources();
        _finalRenderPass.UpdateResources();
        _finalPipeline.UpdateImage(1, _textureImage.GetView(), _textureSampler);
        _finalPipeline.UpdateImage(2, _renderPass.GetColorImage().GetView(), _colorSampler);
    }

    void Cleanup(std::shared_ptr<Gpu> gpu)
    {
        vkDestroySampler(gpu->Device, _colorSampler, nullptr);

        vkDestroySampler(gpu->Device, _textureSampler, nullptr);
    }
};

int main()
{
    try
    {
        RenderEngine renderEngine;
        renderEngine.Run("Render Texture", 640, 480, App());
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
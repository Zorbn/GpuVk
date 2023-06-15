#include "../GpuVk/RenderEngine.hpp"

/*
 * Update:
 * Make a model that swaps between 2 meshes and has 3 instances.
 */

using namespace GpuVk;

struct VertexData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;
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

const std::vector<VertexData> TestVertices = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};

const std::vector<uint16_t> TestIndices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

const std::vector<VertexData> TestVertices2 = {{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}};

const std::vector<uint16_t> TestIndices2 = {0, 1, 2};

class App : public IRenderer
{
    private:
    Pipeline _pipeline;
    RenderPass _renderPass;

    ClearColor _clearColor;

    Image _textureImage;
    Sampler _textureSampler;

    UniformBuffer<UniformBufferData> _ubo;
    UniformBufferData _uboData;
    Model<VertexData, uint16_t, InstanceData> _spriteModel;

    uint32_t _frameCount = 0;

    void UpdateProjectionMatrix(int32_t width, int32_t height)
    {
        _uboData.Proj = glm::perspective(glm::radians(45.0f), width / static_cast<float>(height), 0.1f, 20.0f);
        _uboData.Proj[1][1] *= -1;
    }

    public:
    void Init(std::shared_ptr<Gpu> gpu, SDL_Window* window, int32_t width, int32_t height)
    {
        _textureImage = Image::CreateTexture(gpu, "res/updateImg.png", true);
        _textureSampler = Sampler(gpu, _textureImage);

        _spriteModel = Model<VertexData, uint16_t, InstanceData>::Create(gpu, 3);
        std::vector<InstanceData> instances = {InstanceData{glm::vec3(1.0f, 0.0f, 0.0f)},
            InstanceData{glm::vec3(0.0f, 1.0f, 0.0f)}, InstanceData{glm::vec3(0.0f, 0.0f, 1.0f)}};
        _spriteModel.UpdateInstances(instances);

        _ubo = UniformBuffer<UniformBufferData>(gpu);
        UpdateProjectionMatrix(width, height);

        RenderPassOptions renderPassOptions{};
        renderPassOptions.EnableDepth = true;
        renderPassOptions.ColorAttachmentUsage = ColorAttachmentUsage::PresentWithMsaa;
        _renderPass = RenderPass(gpu, renderPassOptions);

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

        PipelineOptions pipelineOptions{};
        pipelineOptions.VertexShader = "res/updateShader.vert.spv";
        pipelineOptions.FragmentShader = "res/updateShader.frag.spv";
        pipelineOptions.EnableTransparency = false;
        pipelineOptions.VertexDataOptions = vertexDataOptions;
        pipelineOptions.InstanceDataOptions = instanceDataOptions;
        pipelineOptions.DescriptorLayouts.push_back({
            .Binding = 0,
            .Type = DescriptorType::UniformBuffer,
            .ShaderStage = ShaderStage::Vertex,
        });
        pipelineOptions.DescriptorLayouts.push_back({
            .Binding = 1,
            .Type = DescriptorType::ImageSampler,
            .ShaderStage = ShaderStage::Fragment,
        });
        _pipeline = Pipeline(gpu, pipelineOptions, _renderPass);
        _pipeline.UpdateUniform(0, _ubo);
        _pipeline.UpdateImage(1, _textureImage, _textureSampler);
    }

    void Update(std::shared_ptr<Gpu> gpu)
    {
        uint32_t animFrame = _frameCount / 3000;
        if (_frameCount % 3000 == 0)
        {
            if (animFrame % 2 == 0)
                _spriteModel.Update(TestVertices2, TestIndices2);
            else
                _spriteModel.Update(TestVertices, TestIndices);
        }

        _frameCount++;
    }

    void Render(std::shared_ptr<Gpu> gpu)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        _uboData.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        _uboData.View =
            glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        _ubo.Update(_uboData);

        gpu->Commands.BeginBuffer();

        _renderPass.Begin(_clearColor);
        _pipeline.Bind();

        _spriteModel.Draw();

        _renderPass.End();

        gpu->Commands.EndBuffer();
    }

    void Resize(std::shared_ptr<Gpu> gpu, int32_t width, int32_t height)
    {
        UpdateProjectionMatrix(width, height);

        _renderPass.UpdateResources();
    }
};

int main()
{
    try
    {
        RenderEngine renderEngine;
        renderEngine.Run("Update", 640, 480, App(), PresentMode::NoVsync);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
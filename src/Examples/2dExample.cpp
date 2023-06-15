#include "../GpuVk/RenderEngine.hpp"

/*
 * 2d:
 * Render 2d sprites.
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
    glm::vec2 Size;
    glm::vec2 TexPos;
    glm::vec2 TexSize;
};

struct UniformBufferData
{
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};

const std::vector<VertexData> SpriteVertices = {
    {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> SpriteIndices = {0, 2, 1, 0, 3, 2};

class SpriteBatch
{
    private:
    Model<VertexData, uint16_t, InstanceData> _spriteModel;
    std::vector<InstanceData> _instances;

    Image _textureImage;
    Sampler _textureSampler;

    float _inverseImageWidth = 0.0f;
    float _inverseImageHeight = 0.0f;

    public:
    void Init(std::shared_ptr<Gpu> gpu, const std::string& image, size_t maxSprites)
    {
        _textureImage = Image::CreateTexture(gpu, image, false);
        _textureSampler = Sampler(gpu, _textureImage, FilterMode::Nearest, FilterMode::Nearest);

        _inverseImageWidth = 1.0f / _textureImage.GetWidth();
        _inverseImageHeight = 1.0f / _textureImage.GetHeight();

        _spriteModel = Model<VertexData, uint16_t, InstanceData>::FromVerticesAndIndices(
            gpu, SpriteVertices, SpriteIndices, maxSprites);
    }

    void Begin()
    {
        _instances.clear();
    }

    void Add(float x, float y, float depth, float sizeX, float sizeY, float texX, float texY, float texWidth,
        float texHeight)
    {
        _instances.push_back(InstanceData{glm::vec3(x, y, depth), glm::vec2(sizeX, sizeY),
            glm::vec2(texX * _inverseImageWidth, texY * _inverseImageHeight),
            glm::vec2(texWidth * _inverseImageWidth, texHeight * _inverseImageHeight)});
    }

    void End()
    {
        _spriteModel.UpdateInstances(_instances);
    }

    void Draw()
    {
        _spriteModel.Draw();
    }

    const Image& GetImage()
    {
        return _textureImage;
    }

    const Sampler& GetSampler()
    {
        return _textureSampler;
    }
};

class App : public IRenderer
{
    private:
    Pipeline _pipeline;
    RenderPass _renderPass;

    ClearColor _clearColor;

    UniformBuffer<UniformBufferData> _ubo;
    UniformBufferData _uboData;

    SpriteBatch _spriteBatch;

    void UpdateProjectionMatrix(int32_t width, int32_t height)
    {
        _uboData.Proj = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), 0.1f, 10.0f);
    }

    public:
    void Init(std::shared_ptr<Gpu> gpu, SDL_Window* window, int32_t width, int32_t height)
    {
        _ubo = UniformBuffer<UniformBufferData>(gpu);
        _uboData.Model = glm::mat4(1.0f);
        _uboData.View =
            glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        UpdateProjectionMatrix(width, height);

        _spriteBatch.Init(gpu, "res/cubesImg.png", 30);

        RenderPassOptions renderPassOptions{};
        renderPassOptions.EnableDepth = true;
        renderPassOptions.ColorAttachmentUsage = ColorAttachmentUsage::Present;
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
            .Offset = static_cast<uint32_t>(offsetof(InstanceData, Pos)),
        });
        instanceDataOptions.VertexAttributes.push_back({
            .Location = 4,
            .Format = Format::Float2,
            .Offset = static_cast<uint32_t>(offsetof(InstanceData, Size)),
        });
        instanceDataOptions.VertexAttributes.push_back({
            .Location = 5,
            .Format = Format::Float2,
            .Offset = static_cast<uint32_t>(offsetof(InstanceData, TexPos)),
        });
        instanceDataOptions.VertexAttributes.push_back({
            .Location = 6,
            .Format = Format::Float2,
            .Offset = static_cast<uint32_t>(offsetof(InstanceData, TexSize)),
        });

        PipelineOptions pipelineOptions{};
        pipelineOptions.VertexShader = "res/2dShader.vert.spv";
        pipelineOptions.FragmentShader = "res/2dShader.frag.spv";
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
        _pipeline.UpdateImage(1, _spriteBatch.GetImage(), _spriteBatch.GetSampler());
    }

    void Update(std::shared_ptr<Gpu> gpu)
    {
        _spriteBatch.Begin();

        _spriteBatch.Add(0, 0, 0, 32, 16, 0, 16, 32, 16);
        _spriteBatch.Add(16, 0, -1, 64, 32, 0, 16, 32, 16);

        _spriteBatch.End();
    }

    void Render(std::shared_ptr<Gpu> gpu)
    {
        _ubo.Update(_uboData);

        gpu->Commands.BeginBuffer();

        _renderPass.Begin(_clearColor);
        _pipeline.Bind();

        _spriteBatch.Draw();

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
        renderEngine.Run("2d", 640, 480, App());
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
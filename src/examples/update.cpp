#include "../vkFrame/renderer.hpp"

/*
 * Update:
 * Make a model that swaps between 2 meshes and has 3 instances.
 */

struct VertexData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;

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
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexData, TexCoord);

        return attributeDescriptions;
    }
};

struct InstanceData
{
    public:
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

const std::vector<VertexData> TestVertices = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};

const std::vector<uint16_t> TestIndices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

const std::vector<VertexData> TestVertices2 = {{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}};

const std::vector<uint16_t> TestIndices2 = {0, 1, 2};

class App
{
    private:
    Pipeline _pipeline;
    RenderPass _renderPass;

    Image _textureImage;
    VkImageView _textureImageView;
    VkSampler _textureSampler;

    UniformBuffer<UniformBufferData> _ubo;
    Model<VertexData, uint16_t, InstanceData> _spriteModel;

    uint32_t _frameCount = 0;

    std::vector<VkClearValue> _clearValues;

    public:
    void Init(VulkanState &vulkanState, SDL_Window *window, int32_t width, int32_t height)
    {
        (void)window;

        vulkanState.Swapchain.Create(
            vulkanState.Device, vulkanState.PhysicalDevice, vulkanState.Surface, width, height);

        vulkanState.Commands.CreatePool(vulkanState.PhysicalDevice, vulkanState.Device, vulkanState.Surface);
        vulkanState.Commands.CreateBuffers(vulkanState.Device, vulkanState.MaxFramesInFlight);

        _textureImage = Image::CreateTexture("res/updateImg.png", vulkanState.Allocator, vulkanState.Commands,
            vulkanState.GraphicsQueue, vulkanState.Device, true);
        _textureImageView = _textureImage.CreateTextureView(vulkanState.Device);
        _textureSampler = _textureImage.CreateTextureSampler(vulkanState.PhysicalDevice, vulkanState.Device);

        _spriteModel = Model<VertexData, uint16_t, InstanceData>::Create(3, vulkanState.Allocator, vulkanState.Commands);
        std::vector<InstanceData> instances = {InstanceData{glm::vec3(1.0f, 0.0f, 0.0f)},
            InstanceData{glm::vec3(0.0f, 1.0f, 0.0f)}, InstanceData{glm::vec3(0.0f, 0.0f, 1.0f)}};
        _spriteModel.UpdateInstances(
            instances, vulkanState.Commands, vulkanState.Allocator, vulkanState.GraphicsQueue, vulkanState.Device);

        _ubo.Create(vulkanState.MaxFramesInFlight, vulkanState.Allocator);

        _renderPass.Create(
            vulkanState.PhysicalDevice, vulkanState.Device, vulkanState.Allocator, vulkanState.Swapchain, true, true);

        _pipeline.CreateDescriptorSetLayout(vulkanState.Device,
            [&](std::vector<VkDescriptorSetLayoutBinding> &bindings)
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
        _pipeline.CreateDescriptorPool(vulkanState.MaxFramesInFlight, vulkanState.Device,
            [&](std::vector<VkDescriptorPoolSize> poolSizes)
            {
                poolSizes.resize(2);
                poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                poolSizes[0].descriptorCount = static_cast<uint32_t>(vulkanState.MaxFramesInFlight);
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(vulkanState.MaxFramesInFlight);
            });
        _pipeline.CreateDescriptorSets(vulkanState.MaxFramesInFlight, vulkanState.Device,
            [&](std::vector<VkWriteDescriptorSet> &descriptorWrites, VkDescriptorSet descriptorSet, uint32_t i)
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

                vkUpdateDescriptorSets(vulkanState.Device, static_cast<uint32_t>(descriptorWrites.size()),
                    descriptorWrites.data(), 0, nullptr);
            });
        _pipeline.Create<VertexData, InstanceData>(
            "res/updateShader.vert.spv", "res/updateShader.frag.spv", vulkanState.Device, _renderPass, false);

        _clearValues.resize(2);
        _clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        _clearValues[1].depthStencil = {1.0f, 0};
    }

    void Update(VulkanState &vulkanState)
    {
        uint32_t animFrame = _frameCount / 3000;
        if (_frameCount % 3000 == 0)
        {
            if (animFrame % 2 == 0)
            {
                _spriteModel.Update(TestVertices2, TestIndices2, vulkanState.Commands, vulkanState.Allocator,
                    vulkanState.GraphicsQueue, vulkanState.Device);
            }
            else
            {
                _spriteModel.Update(TestVertices, TestIndices, vulkanState.Commands, vulkanState.Allocator,
                    vulkanState.GraphicsQueue, vulkanState.Device);
            }
        }

        _frameCount++;
    }

    void Render(VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
    {
        const VkExtent2D &extent = vulkanState.Swapchain.GetExtent();

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferData uboData{};
        uboData.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.View =
            glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uboData.Proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 10.0f);
        uboData.Proj[1][1] *= -1;

        _ubo.Update(uboData);

        vulkanState.Commands.BeginBuffer(currentFrame);

        _renderPass.Begin(imageIndex, commandBuffer, extent, _clearValues);
        _pipeline.Bind(commandBuffer, currentFrame);

        _spriteModel.Draw(commandBuffer);

        _renderPass.End(commandBuffer);

        vulkanState.Commands.EndBuffer(currentFrame);
    }

    void Resize(VulkanState &vulkanState, int32_t width, int32_t height)
    {
        (void)width, (void)height;

        _renderPass.Recreate(vulkanState.Device, vulkanState.Swapchain);
    }

    void Cleanup(VulkanState &vulkanState)
    {
        _pipeline.Cleanup(vulkanState.Device);
        _renderPass.Cleanup(vulkanState.Device);

        _ubo.Destroy(vulkanState.Allocator);

        vkDestroySampler(vulkanState.Device, _textureSampler, nullptr);
        vkDestroyImageView(vulkanState.Device, _textureImageView, nullptr);
        _textureImage.Destroy(vulkanState.Allocator);

        _spriteModel.Destroy(vulkanState.Allocator);
    }

    int Run()
    {
        Renderer renderer;

        std::function<void(VulkanState &, SDL_Window *, int32_t, int32_t)> initCallback =
            [&](VulkanState &vulkanState, SDL_Window *window, int32_t width, int32_t height)
        { this->Init(vulkanState, window, width, height); };

        std::function<void(VulkanState &)> updateCallback = [&](VulkanState vulkanState) { this->Update(vulkanState); };

        std::function<void(VulkanState &, VkCommandBuffer, uint32_t, uint32_t)> renderCallback =
            [&](VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
        { this->Render(vulkanState, commandBuffer, imageIndex, currentFrame); };

        std::function<void(VulkanState &, int32_t, int32_t)> resizeCallback =
            [&](VulkanState &vulkanState, int32_t width, int32_t height) { this->Resize(vulkanState, width, height); };

        std::function<void(VulkanState &)> cleanupCallback = [&](VulkanState &vulkanState)
        { this->Cleanup(vulkanState); };

        try
        {
            renderer.Run(
                "Update", 640, 480, 2, initCallback, updateCallback, renderCallback, resizeCallback, cleanupCallback);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
};

int main()
{
    App app;
    return app.Run();
}
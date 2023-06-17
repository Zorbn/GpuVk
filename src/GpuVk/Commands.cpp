#include "Commands.hpp"
#include "Constants.hpp"
#include "Gpu.hpp"

#include <stdexcept>

namespace GpuVk
{
Commands::Commands(std::shared_ptr<Gpu> gpu) : _gpu(gpu)
{
    CreatePool();
    CreateBuffers();
}

Commands::Commands(Commands&& other)
{
    *this = std::move(other);
}

Commands& Commands::operator=(Commands&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_commandPool, other._commandPool);
    std::swap(_buffers, other._buffers);
    std::swap(_currentBufferIndex, other._currentBufferIndex);

    return *this;
}

Commands::~Commands()
{
    if (!_gpu)
        return;

    vkDestroyCommandPool(_gpu->_device, _commandPool, nullptr);
}

void Commands::CreatePool()
{
    QueueFamilyIndices queueFamilyIndices =
        QueueFamilyIndices::FindQueueFamilies(_gpu->_physicalDevice, _gpu->_surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices._graphicsFamily.value();

    if (vkCreateCommandPool(_gpu->_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics command pool!");
    }
}

void Commands::CreateBuffers()
{
    _buffers.resize(MaxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_buffers.size());

    if (vkAllocateCommandBuffers(_gpu->_device, &allocInfo, _buffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

VkCommandBuffer Commands::BeginSingleTime() const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_gpu->_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Commands::EndSingleTime(VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_gpu->_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_gpu->_graphicsQueue);

    vkFreeCommandBuffers(_gpu->_device, _commandPool, 1, &commandBuffer);
}

void Commands::ResetBuffer()
{
    vkResetCommandBuffer(_buffers[_currentBufferIndex], /*VkCommandBufferResetFlagBits*/ 0);
}

void Commands::BeginBuffer()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(_buffers[_currentBufferIndex], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
}

void Commands::EndBuffer()
{
    if (vkEndCommandBuffer(_buffers[_currentBufferIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

const VkCommandBuffer& Commands::GetBuffer() const
{
    return _buffers[_currentBufferIndex];
}
} // namespace GpuVk
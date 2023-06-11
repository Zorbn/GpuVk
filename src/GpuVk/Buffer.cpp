#include "Buffer.hpp"

Buffer::Buffer()
{
}

Buffer::Buffer(VmaAllocator allocator, vk::DeviceSize byteSize, VkBufferUsageFlags usage, bool cpuAccessible)
    : _byteSize(byteSize)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = byteSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (cpuAccessible)
    {
        allocCreateInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    if (byteSize != 0 &&
        vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &_buffer, &_allocation, &_allocInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }
}

void Buffer::CopyTo(VmaAllocator &allocator, VkQueue graphicsQueue, VkDevice device, Commands &commands, Buffer &dst)
{
    if (_byteSize == 0 || dst.GetSize() == 0)
        return;

    VkCommandBuffer commandBuffer = commands.BeginSingleTime(device);

    VkBufferCopy copyRegion{};
    copyRegion.size = dst._byteSize;
    vkCmdCopyBuffer(commandBuffer, _buffer, dst._buffer, 1, &copyRegion);

    commands.EndSingleTime(commandBuffer, graphicsQueue, device);
}

const VkBuffer &Buffer::GetBuffer()
{
    return _buffer;
}

size_t Buffer::GetSize()
{
    return _byteSize;
}

void Buffer::Map(VmaAllocator allocator, void **data)
{
    if (_byteSize == 0)
        return;

    vmaMapMemory(allocator, _allocation, data);
}

void Buffer::Unmap(VmaAllocator allocator)
{
    if (_byteSize == 0)
        return;

    vmaUnmapMemory(allocator, _allocation);
}

void Buffer::Destroy(VmaAllocator &allocator)
{
    if (_byteSize == 0)
        return;

    vmaDestroyBuffer(allocator, _buffer, _allocation);
}

void Buffer::SetData(const void *data)
{
    if (_byteSize == 0)
        return;

    memcpy(_allocInfo.pMappedData, data, _byteSize);
}
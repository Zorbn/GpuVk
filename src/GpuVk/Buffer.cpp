#include "Buffer.hpp"
#include "Gpu.hpp"

Buffer::Buffer(std::shared_ptr<Gpu> gpu, uint64_t byteSize, VkBufferUsageFlags usage, bool cpuAccessible)
    : _gpu(gpu), _byteSize(byteSize)
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
        vmaCreateBuffer(gpu->Allocator, &bufferInfo, &allocCreateInfo, &_buffer, &_allocation, &_allocationInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }
}

Buffer::Buffer(Buffer&& other)
{
    *this = std::move(other);
}

Buffer& Buffer::operator=(Buffer&& other)
{
    std::swap(_gpu, other._gpu);

    std::swap(_buffer, other._buffer);
    std::swap(_allocation, other._allocation);
    std::swap(_allocationInfo, other._allocationInfo);
    std::swap(_byteSize, other._byteSize);

    return *this;
}

Buffer::~Buffer()
{
    if (_byteSize == 0)
        return;

    vmaDestroyBuffer(_gpu->Allocator, _buffer, _allocation);
}

void Buffer::CopyTo(Buffer& dst)
{
    if (_byteSize == 0 || dst.GetSize() == 0)
        return;

    VkCommandBuffer commandBuffer = _gpu->Commands.BeginSingleTime(_gpu->Device);

    VkBufferCopy copyRegion{};
    copyRegion.size = dst._byteSize;
    vkCmdCopyBuffer(commandBuffer, _buffer, dst._buffer, 1, &copyRegion);

    _gpu->Commands.EndSingleTime(commandBuffer, _gpu->GraphicsQueue, _gpu->Device);
}

const VkBuffer& Buffer::GetBuffer() const
{
    return _buffer;
}

size_t Buffer::GetSize() const
{
    return _byteSize;
}

void Buffer::Map(void** data)
{
    if (_byteSize == 0)
        return;

    vmaMapMemory(_gpu->Allocator, _allocation, data);
}

void Buffer::Unmap()
{
    if (_byteSize == 0)
        return;

    vmaUnmapMemory(_gpu->Allocator, _allocation);
}

void Buffer::SetData(const void* data)
{
    if (_byteSize == 0)
        return;

    memcpy(_allocationInfo.pMappedData, data, _byteSize);
}
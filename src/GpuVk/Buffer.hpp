#pragma once

#include <vk_mem_alloc.h>

#include <memory>
#include <stdexcept>
#include <vector>

namespace GpuVk
{
class Gpu;

class Buffer
{
    friend class Image;
    template <typename V, typename I, typename D> friend class Model;
    template <typename T> friend class UniformBuffer;

    public:
    template <typename T> static Buffer FromIndices(std::shared_ptr<Gpu> gpu, const std::vector<T>& indices)
    {
        size_t indexSize = sizeof(T);

        // Only accept 16 or 32 bit types.
        if (indexSize != 2 && indexSize != 4)
            throw std::runtime_error("Incorrect size when creating index buffer, indices should be 16 or 32 bit!");

        VkDeviceSize bufferByteSize = indexSize * indices.size();

        Buffer stagingBuffer(gpu, bufferByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        stagingBuffer.SetData(indices.data());

        Buffer indexBuffer(
            gpu, bufferByteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false);

        stagingBuffer.CopyTo(indexBuffer);

        return indexBuffer;
    }

    template <typename T> static Buffer FromVertices(std::shared_ptr<Gpu> gpu, const std::vector<T>& vertices)
    {
        VkDeviceSize bufferByteSize = sizeof(T) * vertices.size();

        Buffer stagingBuffer(gpu, bufferByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        stagingBuffer.SetData(vertices.data());

        Buffer vertexBuffer(
            gpu, bufferByteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false);

        stagingBuffer.CopyTo(vertexBuffer);

        return vertexBuffer;
    }

    Buffer() = default;
    Buffer(Buffer&& other);
    Buffer& operator=(Buffer&& other);
    ~Buffer();

    void SetData(const void* data);
    void CopyTo(Buffer& dst);
    size_t GetSize() const;
    void Map(void** data);
    void Unmap();

    private:
    Buffer(std::shared_ptr<Gpu> gpu, uint64_t byteSize, VkBufferUsageFlags usage, bool cpuAccessible);

    std::shared_ptr<Gpu> _gpu;

    VkBuffer _buffer;
    VmaAllocation _allocation;
    VmaAllocationInfo _allocationInfo;
    size_t _byteSize = 0;
};
} // namespace GpuVk
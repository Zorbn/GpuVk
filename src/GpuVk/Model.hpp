#pragma once

#include <cinttypes>

template <typename V, typename I, typename D> class Model
{
    public:
    static Model<V, I, D> FromVerticesAndIndices(const std::vector<V>& vertices, const std::vector<I> indices,
        const size_t maxInstances, VmaAllocator allocator, const Commands& commands, VkQueue graphicsQueue, VkDevice device)
    {
        Model model = Create(maxInstances, allocator, commands);
        model._size = indices.size();

        model._indexBuffer = Buffer::FromIndices(allocator, commands, graphicsQueue, device, indices);
        model._vertexBuffer = Buffer::FromVertices(allocator, commands, graphicsQueue, device, vertices);

        return model;
    }

    static Model<V, I, D> Create(const size_t maxInstances, VmaAllocator allocator, const Commands& commands)
    {
        Model model;

        size_t instanceByteSize = maxInstances * sizeof(D);
        model._instanceStagingBuffer = Buffer(allocator, instanceByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        model._instanceBuffer = Buffer(
            allocator, instanceByteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false);

        return model;
    };

    void Draw(const Commands& commands)
    {
        if (_vertexBuffer.GetSize() == 0 || _instanceBuffer.GetSize() == 0 || _indexBuffer.GetSize() == 0)
            return;

        if (_instanceCount < 1)
            return;

        VkIndexType indexType = VK_INDEX_TYPE_UINT16;

        if (sizeof(I) == 4)
            indexType = VK_INDEX_TYPE_UINT32;

        auto commandBuffer = commands.GetBuffer();

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer.GetBuffer(), offsets);
        vkCmdBindVertexBuffers(commandBuffer, 1, 1, &_instanceBuffer.GetBuffer(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.GetBuffer(), 0, indexType);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_size), static_cast<uint32_t>(_instanceCount), 0, 0, 0);
    }

    void Update(const std::vector<V>& vertices, const std::vector<I>& indices, const Commands& commands,
        VmaAllocator allocator, VkQueue graphicsQueue, VkDevice device)
    {
        _size = indices.size();

        vkDeviceWaitIdle(device);

        _indexBuffer.Destroy(allocator);
        _vertexBuffer.Destroy(allocator);

        _indexBuffer = Buffer::FromIndices(allocator, commands, graphicsQueue, device, indices);
        _vertexBuffer = Buffer::FromVertices(allocator, commands, graphicsQueue, device, vertices);
    }

    void UpdateInstances(const std::vector<D>& instances, const Commands& commands, VmaAllocator allocator,
        VkQueue graphicsQueue, VkDevice device)
    {
        _instanceCount = instances.size();
        _instanceStagingBuffer.SetData(instances.data());
        _instanceStagingBuffer.CopyTo(allocator, graphicsQueue, device, commands, _instanceBuffer);
    }

    void Destroy(VmaAllocator allocator)
    {
        _vertexBuffer.Destroy(allocator);
        _indexBuffer.Destroy(allocator);
        _instanceStagingBuffer.Destroy(allocator);
        _instanceBuffer.Destroy(allocator);
    }

    private:
    Buffer _vertexBuffer;
    Buffer _indexBuffer;
    Buffer _instanceBuffer;
    Buffer _instanceStagingBuffer;
    size_t _size = 0;
    size_t _instanceCount = 0;
};
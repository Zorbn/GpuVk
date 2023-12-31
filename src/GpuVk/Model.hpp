#pragma once

#include <cinttypes>

namespace GpuVk
{
template <typename V, typename I, typename D> class Model
{
    public:
    Model() = default;

    static Model<V, I, D> FromVerticesAndIndices(std::shared_ptr<Gpu> gpu, const std::vector<V>& vertices,
        const std::vector<I> indices, const size_t maxInstanceCount)
    {
        Model model(gpu, maxInstanceCount);
        model._size = indices.size();

        model._indexBuffer = Buffer::FromIndices(gpu, indices);
        model._vertexBuffer = Buffer::FromVertices(gpu, vertices);

        return model;
    }

    Model(std::shared_ptr<Gpu> gpu, const size_t maxInstanceCount) : _gpu(gpu)
    {
        size_t instanceByteSize = maxInstanceCount * sizeof(D);
        _instanceStagingBuffer = Buffer(gpu, instanceByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        _instanceBuffer =
            Buffer(gpu, instanceByteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false);
    }

    void Draw()
    {
        if (_vertexBuffer.GetSize() == 0 || _instanceBuffer.GetSize() == 0 || _indexBuffer.GetSize() == 0)
            return;

        if (_instanceCount < 1)
            return;

        VkIndexType indexType = VK_INDEX_TYPE_UINT16;

        if (sizeof(I) == 4)
            indexType = VK_INDEX_TYPE_UINT32;

        auto commandBuffer = _gpu->Commands.GetBuffer();

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, offsets);
        vkCmdBindVertexBuffers(commandBuffer, 1, 1, &_instanceBuffer._buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer._buffer, 0, indexType);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_size), static_cast<uint32_t>(_instanceCount), 0, 0, 0);
    }

    void Update(const std::vector<V>& vertices, const std::vector<I>& indices)
    {
        _size = indices.size();

        vkDeviceWaitIdle(_gpu->_device);

        _indexBuffer = Buffer::FromIndices(_gpu, indices);
        _vertexBuffer = Buffer::FromVertices(_gpu, vertices);
    }

    void UpdateInstances(const std::vector<D>& instances)
    {
        _instanceCount = instances.size();
        _instanceStagingBuffer.SetData(instances.data());
        _instanceStagingBuffer.CopyTo(_instanceBuffer);
    }

    private:
    std::shared_ptr<Gpu> _gpu;

    Buffer _vertexBuffer;
    Buffer _indexBuffer;
    Buffer _instanceBuffer;
    Buffer _instanceStagingBuffer;
    size_t _size = 0;
    size_t _instanceCount = 0;
};
} // namespace GpuVk
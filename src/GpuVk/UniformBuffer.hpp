#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Buffer.hpp"
#include "Constants.hpp"

namespace GpuVk
{
template <typename T> class UniformBuffer
{
    friend class Pipeline;

    public:
    UniformBuffer() = default;

    UniformBuffer(std::shared_ptr<Gpu> gpu)
    {
        VkDeviceSize bufferByteSize = sizeof(T);

        _buffers.resize(MaxFramesInFlight);
        _buffersMapped.resize(MaxFramesInFlight);

        for (size_t i = 0; i < MaxFramesInFlight; i++)
        {
            _buffers[i] = Buffer(gpu, bufferByteSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
            _buffers[i].Map(&_buffersMapped[i]);
        }
    }

    UniformBuffer(UniformBuffer&& other)
    {
        *this = std::move(other);
    }

    UniformBuffer& operator=(UniformBuffer&& other)
    {
        std::swap(_buffers, other._buffers);
        std::swap(_buffersMapped, other._buffersMapped);

        return *this;
    }

    ~UniformBuffer()
    {
        size_t bufferCount = _buffers.size();
        for (size_t i = 0; i < bufferCount; i++)
            _buffers[i].Unmap();
    }

    void Update(const T& data)
    {
        size_t bufferCount = _buffersMapped.size();
        for (size_t i = 0; i < bufferCount; i++)
            memcpy(_buffersMapped[i], &data, sizeof(T));
    }

    size_t GetDataSize() const
    {
        return sizeof(T);
    }

    private:
    const VkBuffer& GetBuffer(uint32_t i) const
    {
        return _buffers[i]._buffer;
    }

    std::vector<Buffer> _buffers;
    std::vector<void*> _buffersMapped;
};
} // namespace GpuVk
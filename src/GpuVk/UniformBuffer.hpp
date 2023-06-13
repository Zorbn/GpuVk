#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Buffer.hpp"
#include "Constants.hpp"

template <typename T> class UniformBuffer
{
    public:
    void Create(VmaAllocator allocator)
    {
        VkDeviceSize bufferByteSize = sizeof(T);

        _buffers.resize(MaxFramesInFlight);
        _buffersMapped.resize(MaxFramesInFlight);

        for (size_t i = 0; i < MaxFramesInFlight; i++)
        {
            _buffers[i] = Buffer(allocator, bufferByteSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
            _buffers[i].Map(allocator, &_buffersMapped[i]);
        }
    }

    void Update(const T& data)
    {
        size_t bufferCount = _buffersMapped.size();
        for (size_t i = 0; i < bufferCount; i++)
        {
            memcpy(_buffersMapped[i], &data, sizeof(T));
        }
    }

    const VkBuffer& GetBuffer(uint32_t i) const
    {
        return _buffers[i].GetBuffer();
    }

    size_t GetDataSize() const
    {
        return sizeof(T);
    }

    void Destroy(VmaAllocator allocator)
    {
        size_t bufferCount = _buffers.size();
        for (size_t i = 0; i < bufferCount; i++)
        {
            _buffers[i].Unmap(allocator);
            _buffers[i].Destroy(allocator);
        }
    }

    private:
    std::vector<Buffer> _buffers;
    std::vector<void*> _buffersMapped;
};
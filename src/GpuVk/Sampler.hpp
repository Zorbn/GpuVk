#pragma once

#include "Image.hpp"
#include "FilterMode.hpp"

class Sampler
{
    friend class Pipeline;

    public:
    Sampler() = default;
    Sampler(std::shared_ptr<Gpu> gpu, const Image& image, FilterMode minFilter = FilterMode::Linear,
        FilterMode magFilter = FilterMode::Linear);
    Sampler(Sampler&& other);
    Sampler& operator=(Sampler&& other);
    ~Sampler();

    private:
    static VkFilter GetVkFilter(FilterMode filterMode);

    std::shared_ptr<Gpu> _gpu;

    VkSampler _sampler;
};
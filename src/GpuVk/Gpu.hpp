#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <set>
#include <vector>

#include "Commands.hpp"
#include "Swapchain.hpp"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

class Gpu
{
    // Classes use Vulkan specific APIs internally, but expose independent APIs.
    // Some of these classes friend each other in order to access Vulkan specific
    // APIs without exposing them to the user.
    friend class RenderEngine;
    friend class RenderPass;
    friend class Swapchain;
    friend class Commands;
    friend class Sampler;
    friend class Swapchain;
    friend class Image;
    friend class Pipeline;
    friend class Buffer;
    template <typename V, typename I, typename D> friend class Model;

    public:
    Swapchain Swapchain;
    Commands Commands;

    private:
    void Init(SDL_Window* window);
    void Cleanup();

    void IncrementFrame();
    VkSemaphore GetCurrentImageAvailableSemaphore() const;
    VkSemaphore GetCurrentRenderFinishedSemaphore() const;
    const VkFence& GetCurrentInFlightFence() const;

    void CreateInstance(SDL_Window* window);
    void CreateAllocator();
    void CreateSyncObjects();
    void SetupDebugMessenger();
    void CreateLogicalDevice();
    void CreateSurface(SDL_Window* window);
    void PickPhysicalDevice();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    std::vector<const char*> GetRequiredExtensions(SDL_Window* window);
    bool CheckValidationLayerSupport();
    bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkSurfaceKHR _surface;
    VkQueue _graphicsQueue;
    VmaAllocator _allocator;
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkQueue _presentQueue;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    uint32_t _currentFrame = 0;
};
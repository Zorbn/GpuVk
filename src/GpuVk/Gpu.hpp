#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <set>

#include "Gpu.hpp"
#include "Swapchain.hpp"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

// TODO: Rename this to Gfx, GPU, etc, once de-vulkanification is done.
// TODO: Put external api into Gpu class that is cointained inside of this class, which should be named GpuVk or similar.
class Gpu
{
    friend class RenderEngine;

    public:
    // TODO: Create everything in Gpu for the user, so they don't have to:
    Swapchain Swapchain;
    Commands Commands;

    // TODO: Remove the Vulkan (Vk/Vma) specific fields from the public API. The external API should be agnostic of
    // Vulkan.
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    VkSurfaceKHR Surface;
    VkQueue GraphicsQueue;
    VmaAllocator Allocator;
    VkInstance Instance;
    VkDebugUtilsMessengerEXT DebugMessenger;
    VkQueue PresentQueue;

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
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    uint32_t _currentFrame = 0;
};
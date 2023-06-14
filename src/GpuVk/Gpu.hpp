#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <set>

#include "Commands.hpp"
#include "Swapchain.hpp"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

class Gpu
{
    friend class RenderEngine;
    friend class Swapchain;

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
    bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    uint32_t _currentFrame = 0;
};
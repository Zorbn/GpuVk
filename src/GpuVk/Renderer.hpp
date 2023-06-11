#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../deps/stb_image.h"

#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include "Buffer.hpp"
#include "Commands.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "QueueFamilyIndices.hpp"
#include "Swapchain.hpp"
#include "UniformBuffer.hpp"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

struct VulkanState
{
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
    VkSurfaceKHR Surface;
    VkQueue GraphicsQueue;
    VmaAllocator Allocator;
    Swapchain Swapchain;
    Commands Commands;
    uint32_t MaxFramesInFlight;
};

class Renderer
{
    public:
    void Run(const std::string &windowTitle, const uint32_t windowWidth, const uint32_t windowHeight,
        const uint32_t maxFramesInFlight,
        std::function<void(VulkanState &vulkanState, SDL_Window *window, int32_t width, int32_t height)> initCallback,
        std::function<void(VulkanState &vulkanState)> updateCallback,
        std::function<void(
            VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)>
            renderCallback,
        std::function<void(VulkanState &vulkanState, int32_t width, int32_t height)> resizeCallback,
        std::function<void(VulkanState &vulkanState)> cleanupCallback);

    private:
    SDL_Window *_window;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkQueue _presentQueue;

    VulkanState _vulkanState;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    uint32_t _currentFrame = 0;

    bool _framebufferResized = false;

    void InitWindow(const std::string &windowTitle, const uint32_t windowWidth, const uint32_t windowHeight);
    void InitVulkan(const uint32_t maxFramesInFlight,
        std::function<void(VulkanState &vulkanState, SDL_Window *window, int32_t width, int32_t height)> initCallback);
    void CreateInstance();
    void CreateAllocator();
    void CreateLogicalDevice();

    void MainLoop(std::function<void(VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex,
                      uint32_t currentFrame)>
                      renderCallback,
        std::function<void(VulkanState &vulkanState)> updateCallback,
        std::function<void(VulkanState &vulkanState, int32_t width, int32_t height)> resizeCallback);
    void DrawFrame(std::function<void(VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex,
                       uint32_t currentFrame)>
                       renderCallback,
        std::function<void(VulkanState &vulkanState, int32_t width, int32_t height)> resizeCallback);
    void WaitWhileMinimized();

    void Cleanup(std::function<void(VulkanState &vulkanState)> cleanupCallback);

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void SetupDebugMessenger();

    void CreateSurface();

    void PickPhysicalDevice();

    void CreateSyncObjects();

    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    std::vector<const char *> GetRequiredExtensions();
    bool CheckValidationLayerSupport();

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);
};
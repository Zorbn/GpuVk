#include "Gpu.hpp"
#include "Constants.hpp"

#include <iostream>

const std::vector<const char*> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto function =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (function)
        return function(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (function)
        function(instance, debugMessenger, pAllocator);
}

void Gpu::Init(SDL_Window* window)
{
    CreateInstance(window);
    SetupDebugMessenger();
    CreateSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateAllocator();
    CreateSyncObjects();
}

void Gpu::CreateInstance(SDL_Window* window)
{
    if (EnableValidationLayers && !CheckValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available!");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions(window);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (EnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        createInfo.ppEnabledLayerNames = ValidationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &Instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

void Gpu::CreateAllocator()
{
    VmaVulkanFunctions vkFuncs = {};

    vkFuncs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vkFuncs.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo aci = {};
    aci.vulkanApiVersion = VK_API_VERSION_1_2;
    aci.physicalDevice = PhysicalDevice;
    aci.device = Device;
    aci.instance = Instance;
    aci.pVulkanFunctions = &vkFuncs;

    vmaCreateAllocator(&aci, &Allocator);
}

void Gpu::CreateSyncObjects()
{
    _imageAvailableSemaphores.resize(MaxFramesInFlight);
    _renderFinishedSemaphores.resize(MaxFramesInFlight);
    _inFlightFences.resize(MaxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MaxFramesInFlight; i++)
    {
        if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateFence(Device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

void Gpu::SetupDebugMessenger()
{
    if (!EnableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");
}

void Gpu::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

std::vector<const char*> Gpu::GetRequiredExtensions(SDL_Window* window)
{
    uint32_t extensionCount = 0;
    std::vector<const char*> extensions;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr))
        throw std::runtime_error("Unable to get Vulkan extensions!");

    extensions.resize(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, &extensions[0]))
        throw std::runtime_error("Unable to get Vulkan extensions!");

    if (EnableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool Gpu::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : ValidationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Gpu::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    (void)messageSeverity, (void)messageType, (void)pUserData;

    std::cout << "Validation layer: " << pCallbackData->pMessage << "\n";

    return VK_FALSE;
}

void Gpu::Cleanup()
{
    Swapchain.Cleanup(Device);

    vmaDestroyAllocator(Allocator);

    for (size_t i = 0; i < MaxFramesInFlight; i++)
    {
        vkDestroySemaphore(Device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(Device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(Device, _inFlightFences[i], nullptr);
    }

    Commands.Destroy(Device);

    vkDestroyDevice(Device, nullptr);

    if (EnableValidationLayers)
        DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);

    vkDestroySurfaceKHR(Instance, Surface, nullptr);
    vkDestroyInstance(Instance, nullptr);
}

void Gpu::IncrementFrame()
{
    _currentFrame = (_currentFrame + 1) % MaxFramesInFlight;
    Commands.SetCurrentBufferIndex(_currentFrame);
}

VkSemaphore Gpu::GetCurrentImageAvailableSemaphore() const
{
    return _imageAvailableSemaphores[_currentFrame];
}

VkSemaphore Gpu::GetCurrentRenderFinishedSemaphore() const
{
    return _renderFinishedSemaphores[_currentFrame];
}

const VkFence& Gpu::GetCurrentInFlightFence() const
{
    return _inFlightFences[_currentFrame];
}

void Gpu::CreateSurface(SDL_Window* window)
{
    if (!SDL_Vulkan_CreateSurface(window, Instance, &Surface))
        throw std::runtime_error("Failed to create window surface!");
}

void Gpu::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            PhysicalDevice = device;
            break;
        }
    }

    if (PhysicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to find a suitable GPU!");
}

void Gpu::CreateLogicalDevice()
{
    QueueFamilyIndices indices =
        QueueFamilyIndices::FindQueueFamilies(PhysicalDevice, Surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = DeviceExtensions.data();

    if (EnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        createInfo.ppEnabledLayerNames = ValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) !=
        VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    vkGetDeviceQueue(Device, indices.GraphicsFamily.value(), 0, &GraphicsQueue);
    vkGetDeviceQueue(Device, indices.PresentFamily.value(), 0, &PresentQueue);
}

bool Gpu::IsDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilies(device, Surface);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate;
    if (extensionsSupported)
    {
        SwapchainSupportDetails swapchainSupport =
            Swapchain.QuerySupport(device, Surface);
        swapChainAdequate = !swapchainSupport.Formats.empty() && !swapchainSupport.PresentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool Gpu::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}
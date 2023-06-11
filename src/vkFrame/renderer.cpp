#include "renderer.hpp"

const std::vector<const char *> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void Renderer::Run(const std::string &windowTitle, const uint32_t windowWidth, const uint32_t windowHeight,
    const uint32_t maxFramesInFlight,
    std::function<void(VulkanState &vulkanState, SDL_Window *window, int32_t width, int32_t height)> initCallback,
    std::function<void(VulkanState &vulkanState)> updateCallback,
    std::function<void(
        VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)>
        renderCallback,
    std::function<void(VulkanState &vulkanState, int32_t width, int32_t height)> resizeCallback,
    std::function<void(VulkanState &vulkanState)> cleanupCallback)
{

    InitWindow(windowTitle, windowWidth, windowHeight);
    InitVulkan(maxFramesInFlight, initCallback);
    MainLoop(renderCallback, updateCallback, resizeCallback);
    Cleanup(cleanupCallback);
}

void Renderer::InitWindow(const std::string &windowTitle, const uint32_t windowWidth, const uint32_t windowHeight)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        throw std::runtime_error("Unable to initialize SDL!");
    }

    if (SDL_Vulkan_LoadLibrary(NULL) != 0)
    {
        throw std::runtime_error("Unable to initialize Vulkan!");
    }

    _window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth,
        windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
}

void Renderer::InitVulkan(const uint32_t maxFramesInFlight,
    std::function<void(VulkanState &vulkanState, SDL_Window *window, int32_t width, int32_t height)> initCallback)
{

    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateAllocator();

    int32_t width;
    int32_t height;
    SDL_Vulkan_GetDrawableSize(_window, &width, &height);

    _vulkanState.MaxFramesInFlight = maxFramesInFlight;

    initCallback(_vulkanState, _window, width, height);

    CreateSyncObjects();
}

void Renderer::CreateAllocator()
{
    VmaVulkanFunctions vkFuncs = {};

    vkFuncs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vkFuncs.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo aci = {};
    aci.vulkanApiVersion = VK_API_VERSION_1_2;
    aci.physicalDevice = _vulkanState.PhysicalDevice;
    aci.device = _vulkanState.Device;
    aci.instance = _instance;
    aci.pVulkanFunctions = &vkFuncs;

    vmaCreateAllocator(&aci, &_vulkanState.Allocator);
}

void Renderer::MainLoop(std::function<void(VulkanState &vulkanState, VkCommandBuffer commandBuffer, uint32_t imageIndex,
                            uint32_t currentFrame)>
                            renderCallback,
    std::function<void(VulkanState &vulkanState)> updateCallback,
    std::function<void(VulkanState &vulkanState, int32_t width, int32_t height)> resizeCallback)
{

    bool isRunning = true;
    SDL_Event event;
    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        _framebufferResized = true;
                    }

                    break;
                case SDL_QUIT:
                    isRunning = false;
                    break;
            }
        }

        updateCallback(_vulkanState);
        DrawFrame(renderCallback, resizeCallback);
    }

    vkDeviceWaitIdle(_vulkanState.Device);
}

void Renderer::WaitWhileMinimized()
{
    int32_t width = 0;
    int32_t height = 0;
    SDL_Vulkan_GetDrawableSize(_window, &width, &height);
    while (width == 0 || height == 0)
    {
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        SDL_WaitEvent(nullptr);
    }
}

void Renderer::Cleanup(std::function<void(VulkanState &vulkanState)> cleanupCallback)
{
    _vulkanState.Swapchain.Cleanup(_vulkanState.Device);

    cleanupCallback(_vulkanState);

    vmaDestroyAllocator(_vulkanState.Allocator);

    for (size_t i = 0; i < _vulkanState.MaxFramesInFlight; i++)
    {
        vkDestroySemaphore(_vulkanState.Device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_vulkanState.Device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_vulkanState.Device, _inFlightFences[i], nullptr);
    }

    _vulkanState.Commands.Destroy(_vulkanState.Device);

    vkDestroyDevice(_vulkanState.Device, nullptr);

    if (EnableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(_instance, _vulkanState.Surface, nullptr);
    vkDestroyInstance(_instance, nullptr);

    SDL_DestroyWindow(_window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

void Renderer::CreateInstance()
{
    if (EnableValidationLayers && !CheckValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

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

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (EnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
        createInfo.ppEnabledLayerNames = ValidationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance!");
    }
}

void Renderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
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

void Renderer::SetupDebugMessenger()
{
    if (!EnableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}

void Renderer::CreateSurface()
{
    if (!SDL_Vulkan_CreateSurface(_window, _instance, &_vulkanState.Surface))
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void Renderer::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            _vulkanState.PhysicalDevice = device;
            break;
        }
    }

    if (_vulkanState.PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void Renderer::CreateLogicalDevice()
{
    QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilies(_vulkanState.PhysicalDevice, _vulkanState.Surface);

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

    if (vkCreateDevice(_vulkanState.PhysicalDevice, &createInfo, nullptr, &_vulkanState.Device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(_vulkanState.Device, indices.GraphicsFamily.value(), 0, &_vulkanState.GraphicsQueue);
    vkGetDeviceQueue(_vulkanState.Device, indices.PresentFamily.value(), 0, &_presentQueue);
}

bool Renderer::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

uint32_t Renderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_vulkanState.PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void Renderer::CreateSyncObjects()
{
    _imageAvailableSemaphores.resize(_vulkanState.MaxFramesInFlight);
    _renderFinishedSemaphores.resize(_vulkanState.MaxFramesInFlight);
    _inFlightFences.resize(_vulkanState.MaxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < _vulkanState.MaxFramesInFlight; i++)
    {
        if (vkCreateSemaphore(_vulkanState.Device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateSemaphore(_vulkanState.Device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateFence(_vulkanState.Device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

void Renderer::DrawFrame(std::function<void(VulkanState &vulkanState, VkCommandBuffer commandBuffer,
                             uint32_t imageIndex, uint32_t currentFrame)>
                             renderCallback,
    std::function<void(VulkanState &vulkanState, int32_t width, int32_t height)> resizeCallback)
{

    vkWaitForFences(_vulkanState.Device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result =
        _vulkanState.Swapchain.GetNextImage(_vulkanState.Device, _imageAvailableSemaphores[_currentFrame], imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        WaitWhileMinimized();
        int32_t width;
        int32_t height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        _vulkanState.Swapchain.Recreate(_vulkanState.Device, _vulkanState.PhysicalDevice, _vulkanState.Surface, width, height);
        resizeCallback(_vulkanState, width, height);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    vkResetFences(_vulkanState.Device, 1, &_inFlightFences[_currentFrame]);

    _vulkanState.Commands.ResetBuffer(_currentFrame);
    const VkCommandBuffer &currentBuffer = _vulkanState.Commands.GetBuffer(_currentFrame);
    renderCallback(_vulkanState, currentBuffer, imageIndex, _currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentBuffer;

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_vulkanState.GraphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_vulkanState.Swapchain.GetSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized)
    {
        _framebufferResized = false;
        WaitWhileMinimized();
        int32_t width;
        int32_t height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        _vulkanState.Swapchain.Recreate(_vulkanState.Device, _vulkanState.PhysicalDevice, _vulkanState.Surface, width, height);
        resizeCallback(_vulkanState, width, height);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % _vulkanState.MaxFramesInFlight;
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilies(device, _vulkanState.Surface);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate;
    if (extensionsSupported)
    {
        SwapchainSupportDetails swapchainSupport = _vulkanState.Swapchain.QuerySupport(device, _vulkanState.Surface);
        swapChainAdequate = !swapchainSupport.Formats.empty() && !swapchainSupport.PresentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

std::vector<const char *> Renderer::GetRequiredExtensions()
{
    uint32_t extensionCount = 0;
    std::vector<const char *> extensions;
    if (!SDL_Vulkan_GetInstanceExtensions(_window, &extensionCount, nullptr))
    {
        throw std::runtime_error("Unable to get Vulkan extensions!");
    }
    extensions.resize(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(_window, &extensionCount, &extensions[0]))
    {
        throw std::runtime_error("Unable to get Vulkan extensions!");
    }

    if (EnableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Renderer::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : ValidationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) return false;
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    (void)messageSeverity, (void)messageType, (void)pCallbackData, (void)pUserData;

    return VK_FALSE;
}
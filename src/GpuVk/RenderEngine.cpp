#include "RenderEngine.hpp"

void RenderEngine::Run(
    const std::string& windowTitle, const uint32_t windowWidth, const uint32_t windowHeight, IRenderer& renderer)
{
    InitWindow(windowTitle, windowWidth, windowHeight);
    InitVulkan(renderer);
    MainLoop(renderer);
    Cleanup(renderer);
}

void RenderEngine::InitWindow(const std::string& windowTitle, const uint32_t windowWidth, const uint32_t windowHeight)
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

void RenderEngine::InitVulkan(IRenderer& renderer)
{
    _vulkanState.Init(_window);

    int32_t width;
    int32_t height;
    SDL_Vulkan_GetDrawableSize(_window, &width, &height);

    renderer.Init(_vulkanState.Gpu, _vulkanState, _window, width, height);
}

void RenderEngine::MainLoop(IRenderer& renderer)
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

        renderer.Update(_vulkanState.Gpu);
        DrawFrame(renderer);
    }

    vkDeviceWaitIdle(_vulkanState.Device);
}

void RenderEngine::WaitWhileMinimized()
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

void RenderEngine::Cleanup(IRenderer& renderer)
{
    renderer.Cleanup(_vulkanState.Gpu, _vulkanState);
    _vulkanState.Cleanup();

    SDL_DestroyWindow(_window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

void RenderEngine::DrawFrame(IRenderer& renderer)
{
    vkWaitForFences(_vulkanState.Device, 1, &_vulkanState.GetCurrentInFlightFence(), VK_TRUE, UINT64_MAX);

    VkResult result = _vulkanState.Gpu.Swapchain.GetNextImage(
        _vulkanState.Device, _vulkanState.GetCurrentImageAvailableSemaphore());

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        WaitWhileMinimized();
        int32_t width;
        int32_t height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        _vulkanState.Gpu.Swapchain.Recreate(
            _vulkanState.Device, _vulkanState.PhysicalDevice, _vulkanState.Surface, width, height);
        renderer.Resize(_vulkanState.Gpu, _vulkanState, width, height);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    vkResetFences(_vulkanState.Device, 1, &_vulkanState.GetCurrentInFlightFence());

    _vulkanState.Gpu.Commands.ResetBuffer();
    const VkCommandBuffer& currentBuffer = _vulkanState.Gpu.Commands.GetBuffer();
    renderer.Render(_vulkanState.Gpu);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_vulkanState.GetCurrentImageAvailableSemaphore()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentBuffer;

    VkSemaphore signalSemaphores[] = {_vulkanState.GetCurrentRenderFinishedSemaphore()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_vulkanState.GraphicsQueue, 1, &submitInfo, _vulkanState.GetCurrentInFlightFence()) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_vulkanState.Gpu.Swapchain.GetSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &_vulkanState.Gpu.Swapchain.GetCurrentImageIndex();

    result = vkQueuePresentKHR(_vulkanState.PresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized)
    {
        _framebufferResized = false;
        WaitWhileMinimized();
        int32_t width;
        int32_t height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        _vulkanState.Gpu.Swapchain.Recreate(
            _vulkanState.Device, _vulkanState.PhysicalDevice, _vulkanState.Surface, width, height);
        renderer.Resize(_vulkanState.Gpu, _vulkanState, width, height);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    _vulkanState.IncrementFrame();
}
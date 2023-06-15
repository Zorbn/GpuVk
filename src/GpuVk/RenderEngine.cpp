#include "RenderEngine.hpp"

namespace GpuVk
{
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

void RenderEngine::InitVulkan(PresentMode preferredPresentMode, const uint32_t windowWidth, const uint32_t windowHeight)
{
    _gpu = std::make_shared<Gpu>(Gpu());
    _gpu->Init(_window);

    _gpu->Swapchain = Swapchain(_gpu, windowWidth, windowHeight, preferredPresentMode);
    _gpu->Commands = Commands(_gpu);
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

        renderer.Update(_gpu);
        DrawFrame(renderer);
    }

    vkDeviceWaitIdle(_gpu->_device);
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
    _gpu->Cleanup();

    SDL_DestroyWindow(_window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

void RenderEngine::DrawFrame(IRenderer& renderer)
{
    vkWaitForFences(_gpu->_device, 1, &_gpu->GetCurrentInFlightFence(), VK_TRUE, UINT64_MAX);

    VkResult result = _gpu->Swapchain.GetNextImage();

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        WaitWhileMinimized();
        int32_t width;
        int32_t height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        _gpu->Swapchain.Resize(width, height);
        renderer.Resize(_gpu, width, height);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    vkResetFences(_gpu->_device, 1, &_gpu->GetCurrentInFlightFence());

    _gpu->Commands.ResetBuffer();
    const VkCommandBuffer& currentBuffer = _gpu->Commands.GetBuffer();
    renderer.Render(_gpu);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_gpu->GetCurrentImageAvailableSemaphore()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentBuffer;

    VkSemaphore signalSemaphores[] = {_gpu->GetCurrentRenderFinishedSemaphore()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_gpu->_graphicsQueue, 1, &submitInfo, _gpu->GetCurrentInFlightFence()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_gpu->Swapchain._swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &_gpu->Swapchain._currentImageIndex;

    result = vkQueuePresentKHR(_gpu->_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized)
    {
        _framebufferResized = false;
        WaitWhileMinimized();
        int32_t width;
        int32_t height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);
        _gpu->Swapchain.Resize(width, height);
        renderer.Resize(_gpu, width, height);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    _gpu->IncrementFrame();
}
} // namespace GpuVk
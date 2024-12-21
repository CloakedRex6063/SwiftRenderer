#include "Vulkan/VulkanRender.hpp"

vk::Result Swift::Vulkan::Render::Present(
    const vk::Queue queue,
    vk::Semaphore semaphore,
    vk::SwapchainKHR swapchain,
    u32 index)
{
    auto presentInfo =
        vk::PresentInfoKHR().setImageIndices(index).setWaitSemaphores(semaphore).setSwapchains(swapchain);
    auto result = vkQueuePresentKHR(queue, reinterpret_cast<VkPresentInfoKHR*>(&presentInfo));
    return static_cast<vk::Result>(result);
}
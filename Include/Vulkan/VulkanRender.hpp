#pragma once

namespace Swift::Vulkan::Render
{
    vk::Result Present(
        vk::Queue queue,
        vk::Semaphore semaphore,
        vk::SwapchainKHR swapchain,
        u32 index);
}

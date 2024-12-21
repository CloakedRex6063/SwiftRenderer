#pragma once
#include "VulkanStructs.hpp"

namespace Swift::Vulkan::Init
{
    vk::Instance CreateInstance(
        std::string_view appName,
        std::string_view engineName);

    vk::SurfaceKHR CreateSurface(
        vk::Instance instance,
        HWND hwnd);

    vk::PhysicalDevice ChooseGPU(
        const vk::Instance& instance,
        const vk::SurfaceKHR& surface);

    vk::Device CreateDevice(
        vk::PhysicalDevice physicalDevice,
        vk::SurfaceKHR surface);
    VmaAllocator CreateAllocator(const Context& context);

    vk::SwapchainKHR CreateSwapchain(
        const Context& context,
        vk::Extent2D extent,
        u32 queueIndex);

    vk::Queue GetQueue(
        vk::Device device,
        u32 queueFamilyIndex);

    vk::ImageView CreateImageView(
        const Context& context,
        const vk::Image& image,
        const vk::Format& format,
        vk::ImageViewType viewType,
        vk::ImageAspectFlags aspectMask,
        std::string_view debugName = "Image View");

    std::tuple<
        vk::Image,
        VmaAllocation>
    CreateImage(
        const VmaAllocator& allocator,
        vk::Extent3D extent,
        vk::ImageType imageType,
        vk::Format format,
        vk::ImageUsageFlags usage,
        u32 mipLevels = 1);
    std::vector<Image> CreateSwapchainImages(
        const Context& context,
        const Swapchain& swapchain);

    vk::Fence CreateFence(vk::Device device);
    vk::Semaphore CreateSemaphore(vk::Device device);
    vk::CommandBuffer CreateCommandBuffer(
        vk::Device device,
        vk::CommandPool commandPool);
    vk::CommandPool CreateCommandPool(
        vk::Device device,
        u32 queueIndex);
    vk::DispatchLoaderDynamic CreateDynamicLoader(
        vk::Instance instance,
        vk::Device device);
}  // namespace Swift::Vulkan::Init
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
        const Context& context,
        u32 queueFamilyIndex,
        std::string_view debugName);

    vk::ImageView CreateImageView(
        const Context& context,
        const vk::ImageViewCreateInfo& createInfo,
        std::string_view debugName = "Image View");
    vk::ImageView CreateImageView(
        const Context& context,
        vk::Image image,
        vk::Format format,
        vk::ImageViewType viewType,
        vk::ImageAspectFlags aspectMask,
        std::string_view debugName = "Image View");
    vk::ImageView CreateColorImageView(
        const Context& context,
        vk::Image image,
        vk::Format format,
        std::string_view debugName);
    vk::ImageView CreateDepthImageView(
        const Context& context,
        vk::Image image,
        std::string_view debugName);
    
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
    std::tuple<
        vk::Image,
        VmaAllocation>
    CreateImage(
        const Context& context,
        const VkImageCreateInfo& info);
    std::vector<Image> CreateSwapchainImages(
        const Context& context,
        const Swapchain& swapchain);

    std::tuple<
        vk::Buffer,
        VmaAllocation,
        VmaAllocationInfo>
    CreateBuffer(
        const Context& context,
        u32 queueFamilyIndex,
        vk::DeviceSize size,
        vk::BufferUsageFlags bufferUsageFlags);

    vk::Fence CreateFence(
        const Context& context,
        vk::FenceCreateFlags flags,
        std::string_view debugName);
    vk::Semaphore CreateSemaphore(
        const Context& context,
        std::string_view debugName);
    
    vk::CommandBuffer CreateCommandBuffer(
        const Context& context,
        vk::CommandPool commandPool,
        std::string_view debugName);
    vk::CommandPool CreateCommandPool(
        const Context& context,
        u32 queueIndex,
        std::string_view debugName);
    
    vk::DispatchLoaderDynamic CreateDynamicLoader(
        vk::Instance instance,
        vk::Device device);

    vk::Sampler CreateSampler(vk::Device device);

    vk::DescriptorSetLayout CreateDescriptorSetLayout(vk::Device device);
    vk::DescriptorPool CreateDescriptorPool(vk::Device device);
    vk::DescriptorSet CreateDescriptorSet(
        vk::Device device,
        vk::DescriptorPool descriptorPool,
        vk::DescriptorSetLayout descriptorSetLayout);

    vk::ShaderCreateInfoEXT CreateShader(
        std::span<char> shaderCode,
        vk::ShaderStageFlagBits shaderStage,
        const vk::DescriptorSetLayout& descriptorSetLayout,
        const vk::PushConstantRange& pushConstantRange,
        vk::ShaderCreateFlagsEXT shaderFlags = {},
        vk::ShaderStageFlags nextStage = {});
} // namespace Swift::Vulkan::Init
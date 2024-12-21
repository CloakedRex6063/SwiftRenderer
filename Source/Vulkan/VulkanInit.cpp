#include "Vulkan/VulkanInit.hpp"

#include <complex.h>

#include <Vulkan/VulkanStructs.hpp>
#include <queue>

#include "Vulkan/VulkanUtil.hpp"
#include "vulkan/vulkan_win32.h"

namespace
{
    bool CheckQueueSupport(
        const vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR surface)
    {
        std::optional<u32> graphicsFamily;

        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (const auto [index, queueFamily] : std::views::enumerate(queueFamilies))
        {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                const auto support = physicalDevice.getSurfaceSupportKHR(index, surface);
                VK_ASSERT(support.result, "Failed to get surface support for queue family");
                graphicsFamily = static_cast<u32>(index);
                break;
            }
            // TODO: Check based on dedicated queue support. Async uploads requires a separate queue.
        }

        return graphicsFamily.has_value();
    }

    bool CheckSwapchainSupport(
        const vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR surface)
    {
        const auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
        const auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        VK_ASSERT(formats.result, "Failed to get surface formats");
        VK_ASSERT(presentModes.result, "Failed to get surface presentation modes");
        return !formats.value.empty() && !presentModes.value.empty();
    }

    vk::SurfaceFormatKHR ChooseSwapchainSurfaceFormat(
        const vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR surface)
    {
        const auto [result, formats] = physicalDevice.getSurfaceFormatsKHR(surface);
        const auto iterator = std::ranges::find_if(
            formats,
            [](const vk::SurfaceFormatKHR format)
            {
                constexpr auto idealFormat = vk::Format::eB8G8R8A8Unorm;
                constexpr auto idealColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
                return format.format == idealFormat && format.colorSpace == idealColorSpace;
            });
        if (iterator != formats.end())
            return *iterator;
        return formats[0];
    }

    vk::PresentModeKHR ChooseSwapchainPresentMode(
        const vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR surface)
    {
        const auto [result, modes] = physicalDevice.getSurfacePresentModesKHR(surface);
        VK_ASSERT(result, "Failed to get surface presentation modes");
        constexpr auto idealPresentMode = vk::PresentModeKHR::eMailbox;
        const auto iterator = std::ranges::find_if(
            modes,
            [](const vk::PresentModeKHR mode)
            {
                return mode == idealPresentMode;
            });
        if (iterator != modes.end())
            return idealPresentMode;
        return modes[0];
    }

    u32 ChooseSwapchainImageCount(
        const vk::PhysicalDevice gpu,
        const vk::SurfaceKHR surface)
    {
        const auto [result, capabilities] = gpu.getSurfaceCapabilitiesKHR(surface);
        VK_ASSERT(result, "Failed to get surface capabilities");
        return capabilities.minImageCount + 1;
    }

    vk::SurfaceTransformFlagBitsKHR ChooseSwapchainPreTransform(
        const vk::PhysicalDevice gpu,
        const vk::SurfaceKHR surface)
    {
        const auto [result, capabilities] = gpu.getSurfaceCapabilitiesKHR(surface);
        return capabilities.currentTransform;
    }
}  // namespace

namespace Swift::Vulkan
{
    vk::Instance Init::CreateInstance(
        const std::string_view appName,
        const std::string_view engineName)
    {
        const auto appInfo = vk::ApplicationInfo()
                                 .setApplicationVersion(vk::ApiVersion10)
                                 .setEngineVersion(vk::ApiVersion10)
                                 .setApiVersion(vk::ApiVersion13)
                                 .setPApplicationName(appName.data())
                                 .setPEngineName(engineName.data());

        const std::vector extensions{
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef SWIFT_VULKAN_VALIDATION
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
#ifdef SWIFT_WINDOWS
            "VK_KHR_win32_surface",
#else
            "VK_KHR_xcb_surface",
#endif
        };
        const std::vector layers{
            "VK_LAYER_LUNARG_monitor",
#ifdef SWIFT_VULKAN_VALIDATION
            "VK_LAYER_KHRONOS_validation"
#endif
        };

        const auto createInfo = vk::InstanceCreateInfo()
                                    .setPApplicationInfo(&appInfo)
                                    .setPEnabledExtensionNames(extensions)
                                    .setPEnabledLayerNames(layers);

        auto [result, instance] = createInstance(createInfo);
        VK_ASSERT(result, "Failed to create instance");
        return instance;
    }

    vk::SurfaceKHR Init::CreateSurface(
        const vk::Instance instance,
        const HWND hwnd)
    {
#ifdef SWIFT_WINDOWS
        const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = hwnd};
        VkSurfaceKHR surface;
        vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#endif
        return surface;
    }

    // From
    // https://github.com/BredaUniversityGames/Y2024-25-PR-BB/blob/main/engine/renderer/private/vulkan_context.cpp

    vk::PhysicalDevice Init::ChooseGPU(
        const vk::Instance& instance,
        const vk::SurfaceKHR& surface)
    {
        std::map<vk::PhysicalDevice, u32> scores;

        auto [result, devices] = instance.enumeratePhysicalDevices();
        VK_ASSERT(result, "Failed to enumerate physical devices");
        for (auto device : devices)
        {
            vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDescriptorIndexingFeatures>
                structureChain;

            auto& indexingFeatures = structureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
            auto& deviceFeatures = structureChain.get<vk::PhysicalDeviceFeatures2>();

            vk::PhysicalDeviceProperties deviceProperties;
            device.getProperties(&deviceProperties);
            device.getFeatures2(&deviceFeatures);

            // Failed if geometry shader is not supported.
            if (!deviceFeatures.features.geometryShader)
            {
                continue;
            }

            // Failed if it doesn't support both graphics and present
            if (!CheckQueueSupport(device, surface))
            {
                continue;
            }

            std::array extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

            bool extensionSupported = true;
            auto [result, extensionProps] = device.enumerateDeviceExtensionProperties();
            for (const auto& extension : extensions)
            {
                auto iterator = std::ranges::find_if(
                    extensionProps,
                    [&](const vk::ExtensionProperties& supported)
                    {
                        return std::strcmp(supported.extensionName.data(), extension) == 0;
                    });

                if (iterator == extensionProps.end())
                {
                    extensionSupported = false;
                };
            }

            if (!extensionSupported)
                continue;

            // Check for bindless rendering support.
            if (!indexingFeatures.descriptorBindingPartiallyBound || !indexingFeatures.runtimeDescriptorArray)
            {
                continue;
            }

            // Check support for swap chain.
            if (!CheckSwapchainSupport(device, surface))
            {
                continue;
            }

            // Favor discrete GPUs above all else.
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                scores[device] += 50000;

            // Slightly favor integrated GPUs.
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
                scores[device] += 30000;

            scores[device] += deviceProperties.limits.maxImageDimension2D;
        }
        assert(!scores.empty());
        return scores.rbegin()->first;
    }

    vk::Device Init::CreateDevice(
        const vk::PhysicalDevice physicalDevice,
        const vk::SurfaceKHR surface)
    {
        std::vector extensionNames{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        auto structureChain = vk::StructureChain<
            vk::DeviceCreateInfo,
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan12Features,
            vk::PhysicalDeviceVulkan13Features>();

        auto& features12 = structureChain.get<vk::PhysicalDeviceVulkan12Features>();
        features12.setBufferDeviceAddress(true)
            .setDescriptorBindingPartiallyBound(true)
            .setDescriptorBindingSampledImageUpdateAfterBind(true)
            .setDescriptorBindingStorageBufferUpdateAfterBind(true)
            .setDescriptorBindingUniformBufferUpdateAfterBind(true);

        auto& features13 = structureChain.get<vk::PhysicalDeviceVulkan13Features>();
        features13.setDynamicRendering(true).setSynchronization2(true).setMaintenance4(true);

        constexpr auto deviceFeatures = vk::PhysicalDeviceFeatures()
                                            .setSamplerAnisotropy(true)
                                            .setMultiDrawIndirect(true)
                                            .setShaderSampledImageArrayDynamicIndexing(true)
                                            .setShaderStorageBufferArrayDynamicIndexing(true)
                                            .setShaderUniformBufferArrayDynamicIndexing(true);

        auto& deviceFeatures2 = structureChain.get<vk::PhysicalDeviceFeatures2>();
        deviceFeatures2.setFeatures(deviceFeatures);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        for (const auto family : Util::GetQueueFamilyIndices(physicalDevice, surface))
        {
            auto priority = 1.0f;
            auto queueCreateInfo =
                vk::DeviceQueueCreateInfo().setQueueCount(1).setQueueFamilyIndex(family).setQueuePriorities(priority);
            queueCreateInfos.emplace_back(queueCreateInfo);
        }

        auto& deviceCreateInfo = structureChain.get<vk::DeviceCreateInfo>();
        deviceCreateInfo.setPEnabledExtensionNames(extensionNames).setQueueCreateInfos(queueCreateInfos);
        auto [result, device] = physicalDevice.createDevice(deviceCreateInfo);
        VK_ASSERT(result, "Failed to create device");
        return device;
    }

    VmaAllocator Init::CreateAllocator(const Context& context)
    {
        VmaAllocator allocator;
        const VmaAllocatorCreateInfo createInfo{
            .physicalDevice = context.gpu,
            .device = context.device,
            .instance = context.instance,
            .vulkanApiVersion = vk::ApiVersion13,
        };
        vmaCreateAllocator(&createInfo, &allocator);
        return allocator;
    }

    vk::SwapchainKHR Init::CreateSwapchain(
        const Context& context,
        const vk::Extent2D extent,
        const u32 queueIndex)
    {
        const auto& device = context.device;
        const auto& gpu = context.gpu;
        const auto& surface = context.surface;
        auto [format, colorSpace] = ChooseSwapchainSurfaceFormat(gpu, surface);
        const auto createInfo =
            vk::SwapchainCreateInfoKHR()
                .setSurface(surface)
                .setClipped(true)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(ChooseSwapchainPresentMode(gpu, surface))
                .setImageFormat(format)
                .setImageColorSpace(colorSpace)
                .setMinImageCount(ChooseSwapchainImageCount(gpu, surface))
                .setImageExtent(extent)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
                .setImageSharingMode(vk::SharingMode::eExclusive)
                .setQueueFamilyIndices(queueIndex)
                .setPreTransform(ChooseSwapchainPreTransform(gpu, surface));
        const auto [result, swapchain] = device.createSwapchainKHR(createInfo);
        Util::NameObject(swapchain, "Swapchain", context);
        VK_ASSERT(result, "Failed to create swapchain");
        return swapchain;
    }

    vk::Queue Init::GetQueue(
        const vk::Device device,
        const u32 queueFamilyIndex)
    {
        return device.getQueue(queueFamilyIndex, 0);
    }

    vk::ImageView Init::CreateImageView(
        const Context& context,
        const vk::Image& image,
        const vk::Format& format,
        const vk::ImageViewType viewType,
        const vk::ImageAspectFlags aspectMask,
        const std::string_view debugName)
    {
        const auto createInfo =
            vk::ImageViewCreateInfo().setImage(image).setFormat(format).setViewType(viewType).setSubresourceRange(
                Util::GetImageSubresourceRange(aspectMask));
        const auto [result, imageView] = context.device.createImageView(createInfo);
        VK_ASSERT(result, "Failed to create image view");
        Util::NameObject(imageView, debugName, context);
        return imageView;
    }

    std::tuple<
        vk::Image,
        VmaAllocation>
    Init::CreateImage(
        const VmaAllocator& allocator,
        const vk::Extent3D extent,
        const vk::ImageType imageType,
        const vk::Format format,
        const vk::ImageUsageFlags usage,
        const u32 mipLevels)
    {
        VkImage image;
        VmaAllocation allocation;
        const auto createInfo = vk::ImageCreateInfo()
                                    .setExtent(extent)
                                    .setImageType(imageType)
                                    .setMipLevels(mipLevels)
                                    .setArrayLayers(1)
                                    .setFormat(format)
                                    .setTiling(vk::ImageTiling::eOptimal)
                                    .setInitialLayout(vk::ImageLayout::eUndefined)
                                    .setSharingMode(vk::SharingMode::eExclusive)
                                    .setSamples(vk::SampleCountFlagBits::e1)
                                    .setUsage(usage);
        const auto cCreateInfo = static_cast<VkImageCreateInfo>(createInfo);
        constexpr VmaAllocationCreateInfo allocCreateInfo{
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        const auto result = vmaCreateImage(allocator, &cCreateInfo, &allocCreateInfo, &image, &allocation, nullptr);
        assert(result == VK_SUCCESS);
        return {image, allocation};
    }

    std::vector<Image> Init::CreateSwapchainImages(
        const Context& context,
        const Swapchain& swapchain)
    {
        const auto [result, images] = context.device.getSwapchainImagesKHR(swapchain.swapchain);
        std::vector<Image> swapchainImages;
        swapchainImages.reserve(images.size());
        for (const auto& image : images)
        {
            auto format = ChooseSwapchainSurfaceFormat(context.gpu, context.surface);
            auto imageView = CreateImageView(
                context,
                image,
                format.format,
                vk::ImageViewType::e2D,
                vk::ImageAspectFlagBits::eColor,
                "Swapchain Image View");
            auto newImage = Image().SetImage(image).SetView(imageView).SetFormat(format.format);
            swapchainImages.emplace_back(newImage);
        }
        return swapchainImages;
    }

    vk::Fence Init::CreateFence(const vk::Device device)
    {
        auto [result, fence] = device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
        return fence;
    }

    vk::Semaphore Init::CreateSemaphore(const vk::Device device)
    {
        auto [result, semaphore] = device.createSemaphore(vk::SemaphoreCreateInfo());
        return semaphore;
    }

    vk::CommandBuffer Init::CreateCommandBuffer(
        const vk::Device device,
        const vk::CommandPool commandPool)
    {
        const auto allocateInfo = vk::CommandBufferAllocateInfo()
                                      .setCommandBufferCount(1)
                                      .setCommandPool(commandPool)
                                      .setLevel(vk::CommandBufferLevel::ePrimary);
        auto [result, commandBuffers] = device.allocateCommandBuffers(allocateInfo);
        return commandBuffers.front();
    }

    vk::CommandPool Init::CreateCommandPool(
        const vk::Device device,
        const u32 queueIndex)
    {
        const auto createInfo = vk::CommandPoolCreateInfo()
                                    .setQueueFamilyIndex(queueIndex)
                                    .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        auto [result, commandPool] = device.createCommandPool(createInfo);
        return commandPool;
    }
    vk::DispatchLoaderDynamic Init::CreateDynamicLoader(
        const vk::Instance instance,
        const vk::Device device)
    {
        return vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr, device, vkGetDeviceProcAddr);
    }

}  // namespace Swift::Vulkan
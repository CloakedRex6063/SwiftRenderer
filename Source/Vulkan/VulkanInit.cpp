#include "Vulkan/VulkanInit.hpp"
#include "Utils/FileIO.hpp"
#include "Vulkan/VulkanConstants.hpp"
#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "Vulkan/VulkanUtil.hpp"
#include "dds.hpp"
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
            // TODO: Check based on dedicated queue support. Async uploads requires a separate
            // queue.
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

    vk::SurfaceTransformFlagBitsKHR ChooseSwapchainPreTransform(
        const vk::PhysicalDevice gpu,
        const vk::SurfaceKHR surface)
    {
        const auto [result, capabilities] = gpu.getSurfaceCapabilitiesKHR(surface);
        return capabilities.currentTransform;
    }

    vk::Instance CreateInstance(
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

    vk::SurfaceKHR CreateSurface(
        const Swift::Vulkan::Context& context,
        const Swift::InitInfo& initInfo)
    {
#ifdef SWIFT_WINDOWS
        const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = initInfo.hwnd,
        };
        VkSurfaceKHR surface;
        vkCreateWin32SurfaceKHR(context.instance, &surfaceCreateInfo, nullptr, &surface);
#endif
        return surface;
    }

    // From
    // https://github.com/BredaUniversityGames/Y2024-25-PR-BB/blob/main/engine/renderer/private/vulkan_context.cpp

    vk::PhysicalDevice ChooseGPU(
        const Swift::Vulkan::Context& context,
        const Swift::InitInfo& initInfo)
    {
        std::map<u32, vk::PhysicalDevice> scores;

        auto [result, devices] = context.instance.enumeratePhysicalDevices();
        VK_ASSERT(result, "Failed to enumerate physical devices");
        for (auto device : devices)
        {
            vk::StructureChain<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceDescriptorIndexingFeatures>
                structureChain;

            auto& indexingFeatures =
                structureChain.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
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
            if (!CheckQueueSupport(device, context.surface))
            {
                continue;
            }

            if (!deviceFeatures.features.multiDrawIndirect)
            {
                continue;
            }

            std::vector extensions{
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME,
                VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME};
            if (initInfo.bUsePipelines)
            {
                extensions.emplace_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
            }
            else
            {
                extensions.emplace_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
            }

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
            if (!indexingFeatures.descriptorBindingPartiallyBound ||
                !indexingFeatures.runtimeDescriptorArray)
            {
                continue;
            }

            // Check support for swap chain.
            if (!CheckSwapchainSupport(device, context.surface))
            {
                continue;
            }

            u32 score = 0;
            // Favor discrete GPUs above all else.
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                score += 50000;

            // Slightly favor integrated GPUs.
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
                score += 20000;

            score += deviceProperties.limits.maxImageDimension2D;
            scores.emplace(score, device);
        }
        assert(!scores.empty());
        return scores.rbegin()->second;
    }

    vk::Device CreateDevice(
        const Swift::Vulkan::Context& context,
        const Swift::InitInfo& initInfo)
    {
        std::vector extensionNames{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME,
            VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME};
        if (initInfo.bUsePipelines)
        {
            extensionNames.emplace_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
        }
        else
        {
            extensionNames.emplace_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
        }

        const auto [requireResult, extensions] = context.gpu.enumerateDeviceExtensionProperties();
        VK_ASSERT(requireResult, "Failed to enumerate physical device properties");
        for (const auto& extension : extensionNames)
        {
            const auto iterator = std::ranges::find_if(
                extensions,
                [extension](const vk::ExtensionProperties& supported)
                {
                    return std::strcmp(supported.extensionName.data(), extension) == 0;
                });

            if (iterator == extensions.end())
            {
                VK_ASSERT(vk::Result::eErrorExtensionNotPresent, "Failed to find extension");
            }
        }

        using ShaderVariant = vk::StructureChain<
            vk::DeviceCreateInfo,
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan12Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceShaderObjectFeaturesEXT>;
        using PipelineVariant = vk::StructureChain<
            vk::DeviceCreateInfo,
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan12Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT>;

        std::variant<ShaderVariant, PipelineVariant> structureChain;

        structureChain = initInfo.bUsePipelines
                             ? std::variant<ShaderVariant, PipelineVariant>(PipelineVariant{})
                             : std::variant<ShaderVariant, PipelineVariant>(ShaderVariant{});

        auto& features11 =
            initInfo.bUsePipelines
                ? std::get<PipelineVariant>(structureChain)
                      .get<vk::PhysicalDeviceVulkan11Features>()
                : std::get<ShaderVariant>(structureChain).get<vk::PhysicalDeviceVulkan11Features>();
        features11.setShaderDrawParameters(true).setMultiview(true);

        auto& features12 =
            initInfo.bUsePipelines
                ? std::get<PipelineVariant>(structureChain)
                      .get<vk::PhysicalDeviceVulkan12Features>()
                : std::get<ShaderVariant>(structureChain).get<vk::PhysicalDeviceVulkan12Features>();
        features12.setBufferDeviceAddress(true)
            .setDescriptorBindingPartiallyBound(true)
            .setDescriptorBindingSampledImageUpdateAfterBind(true)
            .setDescriptorBindingStorageBufferUpdateAfterBind(true)
            .setDescriptorBindingStorageImageUpdateAfterBind(true)
            .setDescriptorBindingUniformBufferUpdateAfterBind(true)
            .setRuntimeDescriptorArray(true)
            .setDrawIndirectCount(true);

        auto& features13 =
            initInfo.bUsePipelines
                ? std::get<PipelineVariant>(structureChain)
                      .get<vk::PhysicalDeviceVulkan13Features>()
                : std::get<ShaderVariant>(structureChain).get<vk::PhysicalDeviceVulkan13Features>();
        features13.setDynamicRendering(true).setSynchronization2(true).setMaintenance4(true);

        constexpr auto deviceFeatures = vk::PhysicalDeviceFeatures()
                                            .setSamplerAnisotropy(true)
                                            .setMultiDrawIndirect(true)
                                            .setShaderSampledImageArrayDynamicIndexing(true)
                                            .setShaderStorageBufferArrayDynamicIndexing(true)
                                            .setShaderUniformBufferArrayDynamicIndexing(true)
                                            .setSamplerAnisotropy(true);

        auto& deviceFeatures2 =
            initInfo.bUsePipelines
                ? std::get<PipelineVariant>(structureChain).get<vk::PhysicalDeviceFeatures2>()
                : std::get<ShaderVariant>(structureChain).get<vk::PhysicalDeviceFeatures2>();
        deviceFeatures2.setFeatures(deviceFeatures);

        if (initInfo.bUsePipelines)
        {
            auto& dynamicState3 = std::get<PipelineVariant>(structureChain)
                                      .get<vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT>();
            dynamicState3.setExtendedDynamicState3AlphaToCoverageEnable(true)
                .setExtendedDynamicState3AlphaToOneEnable(true)
                .setExtendedDynamicState3ColorBlendAdvanced(true)
                .setExtendedDynamicState3ColorBlendEnable(true)
                .setExtendedDynamicState3ColorBlendEquation(true)
                .setExtendedDynamicState3ColorWriteMask(true)
                .setExtendedDynamicState3ConservativeRasterizationMode(true)
                .setExtendedDynamicState3DepthClampEnable(true)
                .setExtendedDynamicState3RasterizationSamples(true)
                .setExtendedDynamicState3PolygonMode(true)
                .setExtendedDynamicState3SampleMask(true);
        }

        if (!initInfo.bUsePipelines)
        {
            auto& shaderObjectFeatures = std::get<ShaderVariant>(structureChain)
                                             .get<vk::PhysicalDeviceShaderObjectFeaturesEXT>();
            shaderObjectFeatures.setShaderObject(true);
        }

        const auto indices =
            Swift::Vulkan::Util::GetQueueFamilyIndices(context.gpu, context.surface);
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        auto queueProps = context.gpu.getQueueFamilyProperties();
        std::array<std::vector<float>, 3> priorities;
        for (const auto [index, family] : std::views::enumerate(indices))
        {
            auto queueCount = queueProps[family].queueCount;
            priorities[index].resize(queueCount, 1.0f);
            auto queueCreateInfo = vk::DeviceQueueCreateInfo()
                                       .setQueueCount(queueCount)
                                       .setQueueFamilyIndex(family)
                                       .setQueuePriorities(priorities[index]);
            queueCreateInfos.emplace_back(queueCreateInfo);
        }

        auto& deviceCreateInfo =
            initInfo.bUsePipelines
                ? std::get<PipelineVariant>(structureChain).get<vk::DeviceCreateInfo>()
                : std::get<ShaderVariant>(structureChain).get<vk::DeviceCreateInfo>();
        deviceCreateInfo.setPEnabledExtensionNames(extensionNames)
            .setQueueCreateInfos(queueCreateInfos);
        auto [result, device] = context.gpu.createDevice(deviceCreateInfo);
        VK_ASSERT(result, "Failed to create device");
        return device;
    }

    VmaAllocator CreateAllocator(const Swift::Vulkan::Context& context)
    {
        VmaAllocator allocator;
        const VmaAllocatorCreateInfo createInfo{
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = context.gpu,
            .device = context.device,
            .instance = context.instance,
            .vulkanApiVersion = vk::ApiVersion13,
        };
        vmaCreateAllocator(&createInfo, &allocator);
        return allocator;
    }

    vk::detail::DispatchLoaderDynamic CreateDynamicLoader(const Swift::Vulkan::Context& context)
    {
        return {context.instance, vkGetInstanceProcAddr, context.device, vkGetDeviceProcAddr};
    }

    vk::ImageView CreateImageView(
        const Swift::Vulkan::Context& context,
        const vk::Image image,
        const vk::Format format,
        const vk::ImageViewType viewType,
        const vk::ImageAspectFlags aspectMask,
        const u32 baseMipLevel,
        const std::string_view debugName)
    {
        const auto layers = viewType == vk::ImageViewType::eCube ? 6 : 1;
        const auto viewCreateInfo = vk::ImageViewCreateInfo()
                                        .setImage(image)
                                        .setFormat(format)
                                        .setViewType(viewType)
                                        .setSubresourceRange(
                                            Swift::Vulkan::Util::GetImageSubresourceRange(
                                                aspectMask,
                                                1,
                                                baseMipLevel,
                                                layers));

        const auto [viewResult, imageView] = context.device.createImageView(viewCreateInfo);
        VK_ASSERT(viewResult, "Failed to create image view");
        Swift::Vulkan::Util::NameObject(imageView, debugName, context);
        return imageView;
    }

    vk::ShaderCreateInfoEXT CreateShaderExt(
        const std::span<char> shaderCode,
        const vk::ShaderStageFlagBits shaderStage,
        const vk::DescriptorSetLayout& descriptorSetLayout,
        const vk::PushConstantRange& pushConstantRange,
        const vk::ShaderCreateFlagsEXT shaderFlags = {},
        const vk::ShaderStageFlags nextStage = {})
    {
        auto shaderCreateInfo = vk::ShaderCreateInfoEXT()
                                    .setStage(shaderStage)
                                    .setFlags(shaderFlags)
                                    .setNextStage(nextStage)
                                    .setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
                                    .setPCode(shaderCode.data())
                                    .setCodeSize(shaderCode.size())
                                    .setPName("main")
                                    .setSetLayouts(descriptorSetLayout);
        if (pushConstantRange.size > 0)
        {
            shaderCreateInfo.setPushConstantRanges(pushConstantRange);
        }
        return shaderCreateInfo;
    };

    std::vector<vk::ShaderEXT> CreateGraphicsShaderExt(
        const Swift::Vulkan::Context& context,
        const Swift::Vulkan::BindlessDescriptor& descriptor,
        const vk::PushConstantRange pushConstantRange,
        const std::span<char> vertexCode,
        const std::span<char> fragmentCode)
    {
        const std::array shaderCreateInfo{
            CreateShaderExt(
                vertexCode,
                vk::ShaderStageFlagBits::eVertex,
                descriptor.setLayout,
                pushConstantRange,
                vk::ShaderCreateFlagBitsEXT::eLinkStage,
                vk::ShaderStageFlagBits::eFragment),
            CreateShaderExt(
                fragmentCode,
                vk::ShaderStageFlagBits::eFragment,
                descriptor.setLayout,
                pushConstantRange,
                vk::ShaderCreateFlagBitsEXT::eLinkStage),
        };

        const auto [result, shaders] =
            context.device.createShadersEXT(shaderCreateInfo, nullptr, context.dynamicLoader);
        VK_ASSERT(result, "Failed to create shaders!");
        return shaders;
    }

    std::vector<vk::ShaderEXT> CreateComputeShaderExt(
        const Swift::Vulkan::Context& context,
        const Swift::Vulkan::BindlessDescriptor& descriptor,
        const vk::PushConstantRange pushConstantRange,
        const std::span<char> computeCode)
    {
        const auto shaderCreateInfo = CreateShaderExt(
            computeCode,
            vk::ShaderStageFlagBits::eCompute,
            descriptor.setLayout,
            pushConstantRange);

        const auto [result, shaders] =
            context.device.createShadersEXT(shaderCreateInfo, nullptr, context.dynamicLoader);
        VK_ASSERT(result, "Failed to create shaders!");
        return shaders;
    }

    vk::ShaderModule CreateShaderModule(
        const Swift::Vulkan::Context& context,
        const std::span<char> code)
    {
        const auto createInfo = vk::ShaderModuleCreateInfo()
                                    .setCodeSize(code.size())
                                    .setPCode(reinterpret_cast<u32*>(code.data()));
        const auto [result, module] = context.device.createShaderModule(createInfo);
        VK_ASSERT(result, "Failed to create shader module!");
        return module;
    }

    vk::PipelineShaderStageCreateInfo CreateShaderStage(
        const vk::ShaderStageFlagBits stage,
        const vk::ShaderModule shaderModule)
    {
        const auto shaderCreateInfo = vk::PipelineShaderStageCreateInfo()
                                          .setStage(stage)
                                          .setModule(shaderModule)
                                          .setPName("main");
        return shaderCreateInfo;
    }

    vk::Pipeline CreateGraphicsPipeline(
        const Swift::Vulkan::Context& context,
        const vk::PipelineLayout layout,
        const std::span<char> vertexCode,
        const std::span<char> fragmentCode)
    {
        const std::array shaderModules = {
            CreateShaderModule(context, vertexCode),
            CreateShaderModule(context, fragmentCode),
        };
        const std::array shaderStages = {
            CreateShaderStage(vk::ShaderStageFlagBits::eVertex, shaderModules[0]),
            CreateShaderStage(vk::ShaderStageFlagBits::eFragment, shaderModules[1])};

        constexpr std::array dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };
        const auto dynamicStatesCreateInfo =
            vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

        constexpr auto vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo();
        constexpr auto inputAssemblyCreateInfo =
            vk::PipelineInputAssemblyStateCreateInfo().setTopology(
                vk::PrimitiveTopology::eTriangleList);
        constexpr auto viewport =
            vk::Viewport().setWidth(1280.f).setHeight(720.f).setMinDepth(1.f).setMaxDepth(0.f);
        constexpr auto scissor = vk::Rect2D().setExtent({1280, 720});
        const auto viewportCreateInfo =
            vk::PipelineViewportStateCreateInfo().setViewports(viewport).setScissors(scissor);
        constexpr auto rasterizationCreateInfo =
            vk::PipelineRasterizationStateCreateInfo().setLineWidth(1.f);
        constexpr auto msaaStateCreateInfo =
            vk::PipelineMultisampleStateCreateInfo()
                .setMinSampleShading(1.f)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);
        constexpr auto depthStencilStateCreateInfo =
            vk::PipelineDepthStencilStateCreateInfo()
                .setDepthTestEnable(true)
                .setDepthWriteEnable(true)
                .setDepthCompareOp(vk::CompareOp::eGreaterOrEqual);
        constexpr auto colorBlendAttachmentState =
            vk::PipelineColorBlendAttachmentState()
                .setBlendEnable(true)
                .setColorWriteMask(
                    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                .setAlphaBlendOp(vk::BlendOp::eAdd)
                .setColorBlendOp(vk::BlendOp::eAdd)
                .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
                .setSrcColorBlendFactor(vk::BlendFactor::eOne)
                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
        const auto colorBlendStateCreateInfo =
            vk::PipelineColorBlendStateCreateInfo().setAttachments(colorBlendAttachmentState);

        auto colorFormat = vk::Format::eR16G16B16A16Sfloat;
        const auto renderCreateInfo = vk::PipelineRenderingCreateInfo()
                                          .setColorAttachmentFormats(colorFormat)
                                          .setDepthAttachmentFormat(vk::Format::eD32Sfloat);

        const auto createInfo = vk::GraphicsPipelineCreateInfo()
                                    .setPNext(&renderCreateInfo)
                                    .setLayout(layout)
                                    .setStages(shaderStages)
                                    .setPVertexInputState(&vertexInputCreateInfo)
                                    .setPInputAssemblyState(&inputAssemblyCreateInfo)
                                    .setPViewportState(&viewportCreateInfo)
                                    .setPRasterizationState(&rasterizationCreateInfo)
                                    .setPMultisampleState(&msaaStateCreateInfo)
                                    .setPDynamicState(&dynamicStatesCreateInfo)
                                    .setPDepthStencilState(&depthStencilStateCreateInfo)
                                    .setPColorBlendState(&colorBlendStateCreateInfo);

        const auto [result, graphicsPipeline] =
            context.device.createGraphicsPipeline(nullptr, createInfo);
        VK_ASSERT(result, "Failed to create graphics pipeline!");

        for (const auto& shader : shaderModules)
        {
            context.device.destroy(shader);
        }
        return graphicsPipeline;
    }

    vk::Pipeline CreateComputePipeline(
        const Swift::Vulkan::Context& context,
        const vk::PipelineLayout layout,
        const std::span<char> computeCode)
    {
        const auto shaderModule = CreateShaderModule(context, computeCode);
        const auto stage = CreateShaderStage(vk::ShaderStageFlagBits::eCompute, shaderModule);
        const auto createInfo = vk::ComputePipelineCreateInfo().setLayout(layout).setStage(stage);
        const auto [result, computePipeline] =
            context.device.createComputePipeline(nullptr, createInfo);
        VK_ASSERT(result, "Failed to create compute pipeline!");
        return computePipeline;
    }

    vk::PipelineLayout CreatePipelineLayout(
        Swift::Vulkan::Context context,
        Swift::Vulkan::BindlessDescriptor descriptor,
        vk::PushConstantRange pushConstantRange)
    {
        auto layoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(descriptor.setLayout);
        if (pushConstantRange.size > 0)
        {
            layoutCreateInfo.setPushConstantRanges(pushConstantRange);
        }
        // TODO: cache similar pipeline layouts
        const auto [pipeResult, pipelineLayout] =
            context.device.createPipelineLayout(layoutCreateInfo);
        VK_ASSERT(pipeResult, "Failed to create pipeline layout for shader!");
        return pipelineLayout;
    }
} // namespace

namespace Swift::Vulkan
{
    Context Init::CreateContext(const InitInfo& initInfo)
    {
        Context context;
        context.SetInstance(CreateInstance(initInfo.appName, initInfo.engineName))
            .SetSurface(CreateSurface(context, initInfo))
            .SetGPU(ChooseGPU(context, initInfo))
            .SetDevice(CreateDevice(context, initInfo))
            .SetAllocator(CreateAllocator(context))
            .SetDynamicLoader(CreateDynamicLoader(context));
        return context;
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
                .setMinImageCount(Util::GetSwapchainImageCount(gpu, surface))
                .setImageExtent(extent)
                .setImageArrayLayers(1)
                .setImageUsage(
                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
                .setImageSharingMode(vk::SharingMode::eExclusive)
                .setQueueFamilyIndices(queueIndex)
                .setPreTransform(ChooseSwapchainPreTransform(gpu, surface));
        const auto [result, swapchain] = device.createSwapchainKHR(createInfo);
        Util::NameObject(swapchain, "Swapchain", context);
        VK_ASSERT(result, "Failed to create swapchain");
        return swapchain;
    }

    vk::Queue Init::GetQueue(
        const Context& context,
        const u32 queueFamilyIndex,
        const u32 localIndex,
        const std::string_view debugName)
    {
        const auto queue = context.device.getQueue(queueFamilyIndex, localIndex);
        Util::NameObject(queue, debugName, context);
        return queue;
    }

    Image Init::CreateImage(
        const Context& context,
        VkImageCreateInfo& imageCreateInfo,
        VkImageViewCreateInfo& imageViewCreateInfo,
        const std::string_view debugName)
    {
        constexpr VmaAllocationCreateInfo allocCreateInfo{
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };
        VkImage image;
        VmaAllocation allocation;
        [[maybe_unused]]
        const auto vkResult = vmaCreateImage(
            context.allocator,
            &imageCreateInfo,
            &allocCreateInfo,
            &image,
            &allocation,
            nullptr);
        assert(vkResult == VK_SUCCESS);
        Util::NameObject(
            static_cast<vk::Image>(image),
            std::string(debugName) + std::string(" Image"),
            context);

        imageViewCreateInfo.image = image;
        auto [result, imageView] = context.device.createImageView(imageViewCreateInfo);
        VK_ASSERT(result, "Failed to create image view");
        Util::NameObject(imageView, std::string(debugName) + std::string(" View"), context);

        return Image()
            .SetImage(image)
            .SetAllocation(allocation)
            .SetFormat(static_cast<vk::Format>(imageCreateInfo.format))
            .SetView(imageView)
            .SetExtent(imageCreateInfo.extent);
    }

    Image Init::CreateImage(
        const Context& context,
        const vk::ImageType imageType,
        const vk::Extent3D extent,
        const vk::Format format,
        const vk::ImageUsageFlags usage,
        const u32 mipLevels,
        const vk::ImageCreateFlags flags,
        const std::string_view debugName)
    {
        VkImage image;
        VmaAllocation allocation;
        const auto createInfo =
            vk::ImageCreateInfo()
                .setFlags(flags)
                .setExtent(extent)
                .setImageType(imageType)
                .setMipLevels(mipLevels)
                .setArrayLayers(flags & vk::ImageCreateFlagBits::eCubeCompatible ? 6 : 1)
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
        [[maybe_unused]]
        const auto result = vmaCreateImage(
            context.allocator,
            &cCreateInfo,
            &allocCreateInfo,
            &image,
            &allocation,
            nullptr);
        assert(result == VK_SUCCESS);

        Util::NameObject(
            static_cast<vk::Image>(image),
            std::string(debugName) + std::string(" Image"),
            context);

        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor;
        if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment && usage)
        {
            aspectMask = vk::ImageAspectFlagBits::eDepth;
        }

        auto viewType = vk::ImageViewType::e2D;
        if (extent.depth > 1)
        {
            viewType = vk::ImageViewType::e3D;
        }

        if (flags & vk::ImageCreateFlagBits::eCubeCompatible)
        {
            viewType = vk::ImageViewType::eCube;
        }

        const auto imageView = CreateImageView(
            context,
            image,
            format,
            viewType,
            aspectMask,
            0,
            std::string(debugName) + std::string(" View"));

        return Image()
            .SetImage(image)
            .SetAllocation(allocation)
            .SetFormat(createInfo.format)
            .SetView(imageView)
            .SetExtent(extent);
    }

    std::tuple<
        Image,
        Buffer>
    Init::CreateDDSImage(
        const Context& context,
        Queue transferQueue,
        Command transferCommand,
        const std::filesystem::path& filePath,
        int maxMipLevel,
        const bool loadAllMips,
        const std::string_view debugName)
    {
        dds::Header header = dds::ReadHeader(filePath);

        auto imageCreateInfo = header.GetVulkanImageCreateInfo(
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
        auto imageViewCreateInfo = header.GetVulkanImageViewCreateInfo();

        if (maxMipLevel == -1)
        {
            maxMipLevel = static_cast<int>(header.MipLevels()) - 1;
        }
        maxMipLevel = std::clamp(maxMipLevel, 0, static_cast<int>(header.MipLevels()) - 1);

        const auto minLevel = std::max(0, static_cast<int>(std::log2(header.Width() / 4)) + 1);
        const auto mipCount = loadAllMips ? minLevel - maxMipLevel : 1;
        if (loadAllMips)
        {
            imageCreateInfo.mipLevels = mipCount;
            imageCreateInfo.extent = Util::GetMipExtent(imageCreateInfo.extent, maxMipLevel);
            imageViewCreateInfo.subresourceRange.levelCount = mipCount;
        }

        if (!loadAllMips)
        {
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.extent = Util::GetMipExtent(imageCreateInfo.extent, maxMipLevel);
            imageViewCreateInfo.subresourceRange.levelCount = 1;
        }

        auto image = CreateImage(context, imageCreateInfo, imageViewCreateInfo, debugName)
                         .SetMinLod(static_cast<float>(maxMipLevel))
                         .SetMaxLod(static_cast<float>(header.MipLevels() - 1))
                         .SetURI(filePath.string());

        const auto srcImageBarrier = Util::ImageBarrier(
            image.currentLayout,
            vk::ImageLayout::eTransferDstOptimal,
            image,
            vk::ImageAspectFlagBits::eColor,
            mipCount,
            imageCreateInfo.arrayLayers);
        Util::PipelineBarrier(transferCommand, srcImageBarrier);

        const auto buffer = Util::UploadToImage(
            context,
            transferCommand,
            transferQueue.index,
            header,
            filePath.string(),
            maxMipLevel,
            loadAllMips,
            image);

        const auto dstImageBarrier = Util::ImageBarrier(
            image.currentLayout,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            image,
            vk::ImageAspectFlagBits::eColor,
            mipCount,
            imageCreateInfo.arrayLayers);
        Util::PipelineBarrier(transferCommand, dstImageBarrier);

        return {image, buffer};
    }

    std::tuple<
        Image,
        Image,
        Image,
        Image>
    Init::EquiRectangularToCubemap(
        const Context& context,
        const BindlessDescriptor& descriptor,
        std::vector<Image>& samplerCubes,
        vk::Sampler sampler,
        vk::CommandBuffer graphicsCommand,
        vk::Fence graphicsFence,
        Queue graphicsQueue,
        bool bUsePipelines,
        u32 equiIndex,
        const std::string_view debugName)
    {
        auto captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.f);
        const std::array captureViews = {
            lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f)),
            lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(-1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f)),
            lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)),
            lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, -1.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, -1.0f)),
            lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f),
                glm::vec3(0.0f, -1.0f, 0.0f)),
            lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, -1.0f),
                glm::vec3(0.0f, -1.0f, 0.0f))};

        const auto viewsBuffer = CreateBuffer(
            context,
            graphicsQueue.index,
            captureViews.size() * sizeof(glm::mat4),
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
            false,
            "Capture Views Buffer");
        Util::UploadToBuffer(
            context,
            captureViews.data(),
            viewsBuffer,
            0,
            captureViews.size() * sizeof(glm::mat4));

        const auto addressInfo = vk::BufferDeviceAddressInfo().setBuffer(viewsBuffer);
        struct EquiPushConstant
        {
            glm::mat4 projection;
            u64 viewBuffer;
            u32 equiIndex;
        } equiPC{
            .projection = captureProjection,
            .viewBuffer = context.device.getBufferAddress(addressInfo),
            .equiIndex = equiIndex,
        };

        struct IrradiancePushConstant
        {
            u32 skyboxIndex;
        };

        struct FilterPushConstant
        {
            u32 skyboxIndex;
            float roughness;
        };

        const auto equiShader = CreateGraphicsShader(
            context,
            descriptor,
            bUsePipelines,
            sizeof(EquiPushConstant),
            "cubemap.vert.spv",
            "equiToCubemap.frag.spv",
            "Equi Shader");

        const auto irradianceShader = CreateGraphicsShader(
            context,
            descriptor,
            bUsePipelines,
            sizeof(IrradiancePushConstant),
            "quad.vert.spv",
            "irradiance.frag.spv",
            "Irradiance Shader");

        const auto filterShader = CreateGraphicsShader(
            context,
            descriptor,
            bUsePipelines,
            sizeof(FilterPushConstant),
            "quad.vert.spv",
            "prefilter.frag.spv",
            "Prefilter Shader");

        const auto brdfShader = CreateGraphicsShader(
            context,
            descriptor,
            bUsePipelines,
            0,
            "quad.vert.spv",
            "brdf.frag.spv",
            "BRDF Shader");

        constexpr auto cubemapExtent = vk::Extent2D(1024, 1024);
        const auto skybox = Init::CreateImage(
            context,
            vk::ImageType::e2D,
            vk::Extent3D(cubemapExtent, 1),
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled,
            10,
            vk::ImageCreateFlagBits::eCubeCompatible,
            debugName.data() + std::string(" Cubemap"));

        Util::BeginOneTimeCommand(graphicsCommand);
        Render::BindShader(graphicsCommand, context, descriptor, equiShader, bUsePipelines);
        Render::SetPipelineDefault(context, graphicsCommand, cubemapExtent, bUsePipelines);
        graphicsCommand.pushConstants<EquiPushConstant>(
            equiShader.pipelineLayout,
            vk::ShaderStageFlagBits::eAllGraphics,
            0,
            equiPC);

        Render::BeginRendering(graphicsCommand, skybox.imageView, {}, cubemapExtent, (1 << 6) - 1);

        graphicsCommand.draw(36, 1, 0, 0);

        Render::EndRendering(graphicsCommand);

        Util::EndCommand(graphicsCommand);
        Util::SubmitQueueHost(graphicsQueue, graphicsCommand, graphicsFence);
        Util::WaitFence(context, graphicsFence);
        Util::ResetFence(context, graphicsFence);

        samplerCubes.emplace_back(skybox);
        const auto arrayElement = static_cast<u32>(samplerCubes.size() - 1);
        Util::UpdateDescriptorSamplerCube(
            descriptor.set,
            samplerCubes.back().imageView,
            sampler,
            arrayElement,
            context);

        constexpr vk::Extent2D irradianceExtent = vk::Extent2D(128, 128);
        const auto irradiance = CreateImage(
            context,
            vk::ImageType::e2D,
            vk::Extent3D(128, 128, 1),
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled,
            1,
            vk::ImageCreateFlagBits::eCubeCompatible,
            debugName.data() + std::string(" Irradiance"));

        const IrradiancePushConstant irradianceConstant = {
            .skyboxIndex = arrayElement,
        };

        Util::BeginOneTimeCommand(graphicsCommand);
        Render::BindShader(graphicsCommand, context, descriptor, irradianceShader, bUsePipelines);
        Render::SetPipelineDefault(context, graphicsCommand, cubemapExtent, bUsePipelines);

        Render::BeginRendering(graphicsCommand, irradiance.imageView, {}, {128, 128}, (1 << 6) - 1);

        graphicsCommand.pushConstants<IrradiancePushConstant>(
            irradianceShader.pipelineLayout,
            vk::ShaderStageFlagBits::eAllGraphics,
            0,
            irradianceConstant);

        graphicsCommand.draw(36, 1, 0, 0);

        Render::EndRendering(graphicsCommand);

        constexpr auto specularExtent = vk::Extent2D(256, 256);
        const auto specular = CreateImage(
            context,
            vk::ImageType::e2D,
            vk::Extent3D(specularExtent, 1),
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment |
                vk::ImageUsageFlagBits::eSampled,
            8,
            vk::ImageCreateFlagBits::eCubeCompatible,
            debugName.data() + std::string(" Specular"));

        FilterPushConstant specularConstant = {
            .skyboxIndex = arrayElement,
            .roughness = 0,
        };

        constexpr int specularMipLevels = 4;
        std::array<vk::ImageView, specularMipLevels> specularStuff = {};
        for (int i = 0; i < specularMipLevels; i++)
        {
            specularStuff[i] = CreateImageView(
                context,
                specular,
                vk::Format::eR16G16B16A16Sfloat,
                vk::ImageViewType::e2D,
                vk::ImageAspectFlagBits::eColor,
                i,
                "Specular Mip Stuff");
        }

        Render::BindShader(graphicsCommand, context, descriptor, filterShader, bUsePipelines);
        // for each mip level
        for (int i = 0; i < specularMipLevels; i++)
        {
            auto extent = Util::GetMipExtent(specularExtent, i);
            Render::BeginRendering(graphicsCommand, specularStuff[i], {}, extent, (1 << 6) - 1);
            float roughness = static_cast<float>(i) / (10.f - 1.0f);

            specularConstant.roughness = roughness;

            graphicsCommand.pushConstants<FilterPushConstant>(
                filterShader.pipelineLayout,
                vk::ShaderStageFlagBits::eAllGraphics,
                0,
                specularConstant);

            graphicsCommand.draw(3, 1, 0, 0);
            Render::EndRendering(graphicsCommand);
        }

        const auto lut = CreateImage(
            context,
            vk::ImageType::e2D,
            vk::Extent3D(cubemapExtent, 1),
            vk::Format::eR16G16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            1,
            {},
            debugName.data() + std::string(" Lut"));

        Render::BindShader(graphicsCommand, context, descriptor, brdfShader, bUsePipelines);
        Render::BeginRendering(graphicsCommand, lut.imageView, {}, cubemapExtent, 0);

        Render::SetCullMode(graphicsCommand, vk::CullModeFlagBits::eNone);
        graphicsCommand.draw(3, 1, 0, 0);

        Render::EndRendering(graphicsCommand);

        Util::EndCommand(graphicsCommand);
        Util::SubmitQueueHost(graphicsQueue, graphicsCommand, graphicsFence);
        Util::WaitFence(context, graphicsFence);
        Util::ResetFence(context, graphicsFence);

        equiShader.Destroy(context);
        irradianceShader.Destroy(context);
        filterShader.Destroy(context);
        brdfShader.Destroy(context);
        viewsBuffer.Destroy(context);

        for (int i = 0; i < specularMipLevels; i++)
        {
            context.device.destroy(specularStuff[i]);
        }

        return {skybox, irradiance, specular, lut};
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
                0,
                "Swapchain Image View");
            auto newImage = Image().SetImage(image).SetView(imageView).SetFormat(format.format);
            swapchainImages.emplace_back(newImage);
        }
        return swapchainImages;
    }

    Buffer Init::CreateBuffer(
        const Context& context,
        u32 queueFamilyIndex,
        const vk::DeviceSize size,
        const vk::BufferUsageFlags bufferUsageFlags,
        const bool readback,
        const std::string_view debugName)
    {
        const auto props = context.gpu.getProperties();
        const auto uboAlignment = props.limits.minUniformBufferOffsetAlignment;
        const auto storageAlignment = props.limits.minStorageBufferOffsetAlignment;

        const auto createInfo = vk::BufferCreateInfo()
                                    .setQueueFamilyIndices(queueFamilyIndex)
                                    .setSharingMode(vk::SharingMode::eExclusive)
                                    .setSize(size)
                                    .setUsage(bufferUsageFlags);
        const auto cCreateInfo = static_cast<VkBufferCreateInfo>(createInfo);
        const auto isUBO = bufferUsageFlags & vk::BufferUsageFlagBits::eUniformBuffer;

        VmaAllocationCreateInfo allocCreateInfo;
        if (readback)
        {
            allocCreateInfo = {
                .flags =
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                .requiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            };
        }
        else
        {
            allocCreateInfo = {.usage = VMA_MEMORY_USAGE_CPU_TO_GPU};
        }

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
        [[maybe_unused]]
        const auto result = vmaCreateBufferWithAlignment(
            context.allocator,
            &cCreateInfo,
            &allocCreateInfo,
            isUBO ? uboAlignment : storageAlignment,
            &buffer,
            &allocation,
            &info);
        assert(result == VK_SUCCESS);

        Util::NameObject(static_cast<vk::Buffer>(buffer), debugName, context);
        return Buffer().SetBuffer(buffer).SetAllocation(allocation).SetAllocationInfo(info);
    }

    vk::Fence Init::CreateFence(
        const Context& context,
        const vk::FenceCreateFlags flags,
        const std::string_view debugName)
    {
        auto [result, fence] = context.device.createFence(vk::FenceCreateInfo().setFlags(flags));
        VK_ASSERT(result, "Failed to create fence");
        Util::NameObject(fence, debugName, context);
        return fence;
    }

    vk::Semaphore Init::CreateSemaphore(
        const Context& context,
        const std::string_view debugName)
    {
        auto [result, semaphore] = context.device.createSemaphore(vk::SemaphoreCreateInfo());
        VK_ASSERT(result, "Failed to create semaphore");
        Util::NameObject(semaphore, debugName, context);
        return semaphore;
    }

    vk::CommandBuffer Init::CreateCommandBuffer(
        const Context& context,
        const vk::CommandPool commandPool,
        const std::string_view debugName)
    {
        const auto allocateInfo = vk::CommandBufferAllocateInfo()
                                      .setCommandBufferCount(1)
                                      .setCommandPool(commandPool)
                                      .setLevel(vk::CommandBufferLevel::ePrimary);
        auto [result, commandBuffers] = context.device.allocateCommandBuffers(allocateInfo);
        VK_ASSERT(result, "Failed to allocate command buffers");
        Util::NameObject(commandBuffers.front(), debugName, context);
        return commandBuffers.front();
    }

    vk::CommandPool Init::CreateCommandPool(
        const Context& context,
        const u32 queueIndex,
        const std::string_view debugName)
    {
        const auto createInfo = vk::CommandPoolCreateInfo()
                                    .setQueueFamilyIndex(queueIndex)
                                    .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        auto [result, commandPool] = context.device.createCommandPool(createInfo);
        VK_ASSERT(result, "Failed to create command pool");
        Util::NameObject(commandPool, debugName, context);
        return commandPool;
    }

    vk::Sampler Init::CreateSampler(const Context& context)
    {
        const auto props = context.gpu.getProperties();
        const auto samplerCreateInfo = vk::SamplerCreateInfo()
                                           .setMagFilter(vk::Filter::eLinear)
                                           .setMinFilter(vk::Filter::eLinear)
                                           .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                                           .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                                           .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                                           .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                                           .setUnnormalizedCoordinates(false)
                                           .setCompareOp(vk::CompareOp::eAlways)
                                           .setCompareEnable(false)
                                           .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                           .setMinLod(0)
                                           .setMaxLod(13)
                                           .setAnisotropyEnable(true)
                                           .setMaxAnisotropy(props.limits.maxSamplerAnisotropy);
        const auto [result, sampler] = context.device.createSampler(samplerCreateInfo);
        VK_ASSERT(result, "Failed to create sampler!");
        return sampler;
    }

    vk::DescriptorSetLayout Init::CreateDescriptorSetLayout(const vk::Device device)
    {
        std::array descriptorBindings = {
            vk::DescriptorSetLayoutBinding()
                .setBinding(Constants::SamplerBinding)
                .setDescriptorCount(Constants::MaxSamplerDescriptors)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
            vk::DescriptorSetLayoutBinding()
                .setBinding(Constants::UniformBinding)
                .setDescriptorCount(Constants::MaxUniformDescriptors)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
            vk::DescriptorSetLayoutBinding()
                .setBinding(Constants::StorageBinding)
                .setDescriptorCount(Constants::MaxStorageDescriptors)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
            vk::DescriptorSetLayoutBinding()
                .setBinding(Constants::ImageBinding)
                .setDescriptorCount(Constants::MaxImageDescriptors)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
            vk::DescriptorSetLayoutBinding()
                .setBinding(Constants::CubeSamplerBinding)
                .setDescriptorCount(Constants::MaxCubeSamplerDescriptors)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setStageFlags(vk::ShaderStageFlagBits::eAll),
        };

        constexpr auto flags = vk::DescriptorBindingFlagBits::ePartiallyBound |
                               vk::DescriptorBindingFlagBits::eUpdateAfterBind;
        std::vector bindingsFlags(descriptorBindings.size(), flags);
        const auto bindingInfo =
            vk::DescriptorSetLayoutBindingFlagsCreateInfo().setBindingFlags(bindingsFlags);

        const auto createInfo =
            vk::DescriptorSetLayoutCreateInfo()
                .setPNext(&bindingInfo)
                .setBindings(descriptorBindings)
                .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
        auto [result, layout] = device.createDescriptorSetLayout(createInfo);
        VK_ASSERT(result, "Failed to create descriptor set layout");
        return layout;
    }

    vk::DescriptorPool Init::CreateDescriptorPool(
        const vk::Device device,
        std::span<vk::DescriptorPoolSize> poolSizes)
    {
        if (poolSizes.empty())
        {
            constexpr std::array tempPoolSizes{
                vk::DescriptorPoolSize()
                    .setDescriptorCount(Constants::MaxUniformDescriptors)
                    .setType(vk::DescriptorType::eUniformBuffer),
                vk::DescriptorPoolSize()
                    .setDescriptorCount(Constants::MaxStorageDescriptors)
                    .setType(vk::DescriptorType::eStorageBuffer),
                vk::DescriptorPoolSize()
                    .setDescriptorCount(Constants::MaxSamplerDescriptors)
                    .setType(vk::DescriptorType::eSampler),
                vk::DescriptorPoolSize()
                    .setDescriptorCount(Constants::MaxImageDescriptors)
                    .setType(vk::DescriptorType::eStorageImage),
                vk::DescriptorPoolSize()
                    .setDescriptorCount(Constants::MaxCubeSamplerDescriptors)
                    .setType(vk::DescriptorType::eCombinedImageSampler),
            };
            const auto createInfo =
                vk::DescriptorPoolCreateInfo()
                    .setMaxSets(1)
                    .setPoolSizes(tempPoolSizes)
                    .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
            auto [result, descriptorPool] = device.createDescriptorPool(createInfo);
            VK_ASSERT(result, "Failed to create descriptor pool");
            return descriptorPool;
        }
        const auto createInfo =
            vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizes(poolSizes).setFlags(
                vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);
        auto [result, descriptorPool] = device.createDescriptorPool(createInfo);
        VK_ASSERT(result, "Failed to create descriptor pool");
        return descriptorPool;
    }

    vk::DescriptorSet Init::CreateDescriptorSet(
        const vk::Device device,
        const vk::DescriptorPool descriptorPool,
        vk::DescriptorSetLayout descriptorSetLayout)
    {
        const auto createInfo = vk::DescriptorSetAllocateInfo()
                                    .setDescriptorPool(descriptorPool)
                                    .setDescriptorSetCount(1)
                                    .setSetLayouts(descriptorSetLayout);
        auto [result, descriptorSet] = device.allocateDescriptorSets(createInfo);
        VK_ASSERT(result, "Failed to allocate descriptor set");
        return descriptorSet.front();
    }

    Shader Init::CreateGraphicsShader(
        Context context,
        BindlessDescriptor descriptor,
        bool bUsePipeline,
        u32 pushConstantSize,
        std::string_view vertexPath,
        std::string_view fragmentPath,
        std::string_view debugName)
    {
        const auto pushConstantRange = vk::PushConstantRange()
                                           .setSize(pushConstantSize)
                                           .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics);

        auto vertexCode = FileIO::ReadBinaryFile(vertexPath);
        auto fragmentCode = FileIO::ReadBinaryFile(fragmentPath);

        const auto pipelineLayout = CreatePipelineLayout(context, descriptor, pushConstantRange);

        std::vector<vk::ShaderEXT> shadersExt;
        vk::Pipeline pipeline;
        if (bUsePipeline)
        {
            pipeline = CreateGraphicsPipeline(context, pipelineLayout, vertexCode, fragmentCode);
        }
        else
        {
            shadersExt = CreateGraphicsShaderExt(
                context,
                descriptor,
                pushConstantRange,
                vertexCode,
                fragmentCode);
        }

        const auto stageFlags = {
            vk::ShaderStageFlagBits::eVertex,
            vk::ShaderStageFlagBits::eFragment,
        };

        Util::NameObject(pipelineLayout, debugName, context);

        return Shader()
            .SetShaders(shadersExt)
            .SetPipeline(pipeline)
            .SetPipelineLayout(pipelineLayout)
            .SetStageFlags(stageFlags);
    }

    Shader Init::CreateComputeShader(
        Context context,
        BindlessDescriptor descriptor,
        bool bUsePipeline,
        u32 pushConstantSize,
        std::string_view computePath,
        std::string_view debugName)
    {
        const auto pushConstantRange = vk::PushConstantRange()
                                           .setSize(pushConstantSize)
                                           .setStageFlags(vk::ShaderStageFlagBits::eCompute);

        auto computeCode = FileIO::ReadBinaryFile(computePath);

        const auto pipelineLayout = CreatePipelineLayout(context, descriptor, pushConstantRange);

        std::vector<vk::ShaderEXT> shadersExt;
        vk::Pipeline pipeline;
        if (bUsePipeline)
        {
            pipeline = CreateComputePipeline(context, pipelineLayout, computeCode);
        }
        else
        {
            shadersExt =
                CreateComputeShaderExt(context, descriptor, pushConstantRange, computeCode);
        }

        const auto stageFlags = {vk::ShaderStageFlagBits::eCompute};

        Util::NameObject(pipelineLayout, debugName, context);

        return Shader()
            .SetShaders(shadersExt)
            .SetPipeline(pipeline)
            .SetPipelineLayout(pipelineLayout)
            .SetStageFlags(stageFlags);
    }
} // namespace Swift::Vulkan
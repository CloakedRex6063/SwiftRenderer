#pragma once

namespace Swift::Vulkan
{
    struct Context
    {
        vk::Instance instance;
        vk::SurfaceKHR surface;
        vk::PhysicalDevice gpu;
        vk::Device device;
        VmaAllocator allocator{};
        vk::DispatchLoaderDynamic dynamicLoader;

        operator vk::Device() const { return device; }

        Context& SetInstance(const vk::Instance& instance)
        {
            this->instance = instance;
            return *this;
        }
        Context& SetSurface(const vk::SurfaceKHR& surface)
        {
            this->surface = surface;
            return *this;
        }
        Context& SetGPU(const vk::PhysicalDevice& physicalDevice)
        {
            this->gpu = physicalDevice;
            return *this;
        }
        Context& SetDevice(const vk::Device& device)
        {
            this->device = device;
            return *this;
        }
        Context& SetAllocator(const VmaAllocator& vmaAllocator)
        {
            this->allocator = vmaAllocator;
            return *this;
        }
        Context& SetDynamicLoader(const vk::DispatchLoaderDynamic& dynamicLoader)
        {
            this->dynamicLoader = dynamicLoader;
            return *this;
        }

        void Destroy() const
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            vmaDestroyAllocator(allocator);
            device.destroy();
            instance.destroy();
        };
    };

    struct Queue
    {
        vk::Queue queue;
        u32 index{};

        operator vk::Queue() const { return queue; }

        Queue& SetQueue(const vk::Queue& queue)
        {
            this->queue = queue;
            return *this;
        }
        Queue& SetIndex(const u32 index)
        {
            this->index = index;
            return *this;
        }
    };

    struct Image
    {
        vk::Image image;
        vk::ImageView imageView;
        vk::Format format{};
        VmaAllocation imageAllocation{};
        vk::ImageLayout currentLayout = vk::ImageLayout::eUndefined;

        operator vk::Image() const { return image; }

        Image& SetImage(const vk::Image& image)
        {
            this->image = image;
            return *this;
        }
        Image& SetView(const vk::ImageView& imageView)
        {
            this->imageView = imageView;
            return *this;
        }
        Image& SetFormat(const vk::Format& format)
        {
            this->format = format;
            return *this;
        }
        Image& SetAllocation(const VmaAllocation& imageAllocation)
        {
            this->imageAllocation = imageAllocation;
            return *this;
        }

        void Destroy(const Context& context)
        {
            if (imageAllocation)
            {
                vmaDestroyImage(context.allocator, image, imageAllocation);
                image = nullptr;
                imageAllocation = nullptr;
            }
            if (image)
            {
                context.device.destroyImage(image);
                image = nullptr;
            }
            context.device.destroyImageView(imageView);
        }
        void DestroyView(const Context& context) const
        {
            context.device.destroyImageView(imageView);
        }
    };

    struct Buffer
    {
        vk::Buffer buffer;
        VmaAllocation allocation{};
        VmaAllocationInfo allocationInfo{};

        operator vk::Buffer() const { return buffer; }

        Buffer& SetBuffer(const vk::Buffer& buffer)
        {
            this->buffer = buffer;
            return *this;
        }
        Buffer& SetAllocation(const VmaAllocation& allocation)
        {
            this->allocation = allocation;
            return *this;
        }
        Buffer& SetAllocationInfo(const VmaAllocationInfo& allocationInfo)
        {
            this->allocationInfo = allocationInfo;
            return *this;
        }

        void Destroy(const Context& context) const
        {
            vmaDestroyBuffer(context.allocator, buffer, allocation);
        }
    };

    struct Swapchain
    {
        vk::SwapchainKHR swapchain;
        std::vector<Image> images;
        u32 imageIndex = 0;
        vk::Extent2D extent;
        Image renderImage;
        Image depthImage;

        operator vk::SwapchainKHR() const { return swapchain; }

        Swapchain& SetSwapchain(const vk::SwapchainKHR& swapchain)
        {
            this->swapchain = swapchain;
            return *this;
        }
        Swapchain& SetImages(const std::vector<Image>& images)
        {
            this->images = images;
            return *this;
        }
        Swapchain& SetRenderImage(const Image& renderImage)
        {
            this->renderImage = renderImage;
            return *this;
        }
        Swapchain& SetDepthImage(const Image& depthImage)
        {
            this->depthImage = depthImage;
            return *this;
        }
        Swapchain& SetIndex(const u32 index)
        {
            this->imageIndex = index;
            return *this;
        }
        Swapchain& SetExtent(const vk::Extent2D& extent)
        {
            this->extent = extent;
            return *this;
        }

        void Destroy(const Context& context)
        {
            context.device.destroySwapchainKHR(swapchain);
            renderImage.Destroy(context);
            depthImage.Destroy(context);
            for (auto& image : images)
            {
                image.DestroyView(context);
            }
        }
    };

    struct Command
    {
        vk::CommandPool commandPool;
        vk::CommandBuffer commandBuffer;

        operator vk::CommandBuffer() const { return commandBuffer; }

        void Destroy(const Context& context) const { context.device.destroy(commandPool); }
    };

    struct FrameData
    {
        vk::Semaphore renderSemaphore;
        vk::Semaphore presentSemaphore;
        vk::Fence renderFence;
        Command renderCommand;

        void Destroy(const Context& context) const
        {
            context.device.destroy(renderFence);
            context.device.destroy(renderSemaphore);
            context.device.destroy(presentSemaphore);
            renderCommand.Destroy(context);
        }
    };

    struct BindlessDescriptor
    {
        vk::DescriptorSetLayout setLayout;
        vk::DescriptorSet set;
        vk::DescriptorPool pool;

        BindlessDescriptor&
        SetDescriptorSetLayout(const vk::DescriptorSetLayout& descriptorSetLayout)
        {
            this->setLayout = descriptorSetLayout;
            return *this;
        }
        BindlessDescriptor& SetDescriptorPool(const vk::DescriptorPool& descriptorPool)
        {
            this->pool = descriptorPool;
            return *this;
        }
        BindlessDescriptor& SetDescriptorSet(const vk::DescriptorSet& descriptorSet)
        {
            this->set = descriptorSet;
            return *this;
        }

        void Destroy(const Context& context) const
        {
            context.device.destroyDescriptorSetLayout(setLayout);
            context.device.destroyDescriptorPool(pool);
        }
    };

    struct Shader
    {
        std::vector<vk::ShaderEXT> shaders;
        std::vector<vk::ShaderStageFlagBits> stage;
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;

        Shader& SetShaders(const std::vector<vk::ShaderEXT>& shaders)
        {
            this->shaders = shaders;
            return *this;
        }
        Shader& SetPipeline(const vk::Pipeline& pipeline)
        {
            this->pipeline = pipeline;
            return *this;
        }
        Shader& SetPipelineLayout(const vk::PipelineLayout& pipelineLayout)
        {
            this->pipelineLayout = pipelineLayout;
            return *this;
        }
        Shader& SetStageFlags(const std::vector<vk::ShaderStageFlagBits>& stage)
        {
            this->stage = stage;
            return *this;
        }

        void Destroy(const Context& context) const
        {
            for (const auto& shader : shaders)
            {
                context.device.destroy(shader, nullptr, context.dynamicLoader);
            }
            context.device.destroy(pipeline, nullptr, context.dynamicLoader);
            context.device.destroy(pipelineLayout);
        }
    };
} // namespace Swift::Vulkan
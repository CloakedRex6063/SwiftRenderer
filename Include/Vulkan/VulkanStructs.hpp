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
        Image& SetImageAndAllocation(
            std::tuple<
                vk::Image,
                VmaAllocation> tuple)
        {
            std::tie(image, imageAllocation) = tuple;
            return *this;
        };

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
        void DestroyView(const Context& context) const { context.device.destroyImageView(imageView); }
    };

    struct Swapchain
    {
        vk::SwapchainKHR swapchain;
        std::vector<Image> images;
        Image renderImage;
        Image depthImage;

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

    struct FrameData
    {
        vk::Semaphore renderSemaphore;
        vk::Semaphore presentSemaphore;
        vk::Fence renderFence;
        vk::CommandBuffer commandBuffer;
        vk::CommandPool commandPool;
    };
}  // namespace Swift::Vulkan
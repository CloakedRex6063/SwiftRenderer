#include "Vulkan/VulkanRender.hpp"
#include "Vulkan/VulkanStructs.hpp"
#include "Vulkan/VulkanUtil.hpp"

namespace Swift::Vulkan
{
    u32 Render::AcquireNextImage(
        const Queue& queue,
        const Context& context,
        Swapchain& swapchain,
        const vk::Semaphore semaphore,
        const vk::Extent2D extent)
    {
        const auto acquireInfo = vk::AcquireNextImageInfoKHR()
                                     .setDeviceMask(1)
                                     .setSemaphore(semaphore)
                                     .setSwapchain(swapchain)
                                     .setTimeout(std::numeric_limits<u64>::max());
        auto [result, image] = context.device.acquireNextImage2KHR(acquireInfo);

        if (result == vk::Result::eSuccess)
            return image;

        Util::HandleSubOptimalSwapchain(queue.index, context, swapchain, extent);
        return std::numeric_limits<u32>::max();
    }

    void Render::Present(
        const Context& context,
        Swapchain& swapchain,
        const Queue queue,
        vk::Semaphore semaphore,
        const vk::Extent2D extent)
    {
        auto presentInfo = vk::PresentInfoKHR()
                               .setImageIndices(swapchain.imageIndex)
                               .setWaitSemaphores(semaphore)
                               .setSwapchains(swapchain.swapchain);
        const auto result =
            vkQueuePresentKHR(queue.queue, reinterpret_cast<VkPresentInfoKHR*>(&presentInfo));

        if (result != VK_SUCCESS)
        {
            Util::HandleSubOptimalSwapchain(queue.index, context, swapchain, extent);
        }
    }

    void Render::DefaultRenderConfig(
        const vk::CommandBuffer commandBuffer,
        const vk::Extent2D extent,
        const vk::DispatchLoaderDynamic& dynamicLoader,
        const bool bUsePipeline)
    {
        const auto viewport = vk::Viewport()
                                  .setWidth(extent.width)
                                  .setHeight(extent.height)
                                  .setMinDepth(1.f)
                                  .setMaxDepth(0.f);
        commandBuffer.setViewport(0, viewport);
        commandBuffer.setScissor(0, vk::Rect2D().setExtent(extent));
        if (!bUsePipeline)
        {
            commandBuffer.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
            commandBuffer.setPrimitiveRestartEnable(false);
            commandBuffer.setRasterizerDiscardEnable(false, dynamicLoader);
            commandBuffer.setAlphaToCoverageEnableEXT(false, dynamicLoader);
            commandBuffer.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1, dynamicLoader);
            commandBuffer.setFrontFace(vk::FrontFace::eCounterClockwise);
            commandBuffer.setCullMode(vk::CullModeFlagBits::eBack);
            commandBuffer.setDepthTestEnable(true);
            commandBuffer.setDepthBiasEnable(false);
            commandBuffer.setDepthCompareOp(vk::CompareOp::eGreaterOrEqual);
            commandBuffer.setDepthWriteEnable(true);
            commandBuffer.setStencilTestEnable(false);
            commandBuffer.setPolygonModeEXT(vk::PolygonMode::eFill, dynamicLoader);
            constexpr auto colorWriteMask =
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            constexpr auto colorBlendEquation =
                vk::ColorBlendEquationEXT()
                    .setAlphaBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                    .setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
            commandBuffer.setColorBlendEquationEXT(0, colorBlendEquation, dynamicLoader);
            commandBuffer.setColorBlendEnableEXT(0, true, dynamicLoader);
            commandBuffer.setColorWriteMaskEXT(0, colorWriteMask, dynamicLoader);
            commandBuffer.setSampleMaskEXT(vk::SampleCountFlagBits::e1, 0xFFFFFFFF, dynamicLoader);
            const auto viewport = vk::Viewport()
                                      .setWidth(extent.width)
                                      .setHeight(extent.height)
                                      .setMinDepth(1.f)
                                      .setMaxDepth(0.f);
            const auto scissor = vk::Rect2D().setExtent(extent);
            commandBuffer.setViewportWithCount(viewport);
            commandBuffer.setScissorWithCount(scissor);

            // Thanks khronos group for forcing this
            constexpr auto inputBinding = vk::VertexInputBindingDescription2EXT()
                                              .setBinding(0)
                                              .setInputRate(vk::VertexInputRate::eVertex)
                                              .setDivisor(1)
                                              .setStride(1);
            constexpr auto inputAttribute =
                vk::VertexInputAttributeDescription2EXT().setBinding(0).setFormat(
                    vk::Format::eR32G32B32Sfloat);
            commandBuffer.setVertexInputEXT(inputBinding, inputAttribute, dynamicLoader);
        }
    }
} // namespace Swift::Vulkan
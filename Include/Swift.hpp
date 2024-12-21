#pragma once
#include "Structs.hpp"
#include "Vulkan/VulkanStructs.hpp"

namespace Swift
{
    class Renderer
    {
    public:
        void Init(const InitInfo& initInfo);
        void BeginFrame(const DynamicInfo& dynamicInfo);
        void Render();
        void EndFrame(const DynamicInfo& dynamicInfo);
        void Shutdown();

    private:
        Vulkan::Context mContext;
        Vulkan::Queue mGraphicsQueue;
        Vulkan::Queue mComputeQueue;
        Vulkan::Queue mTransferQueue;
        Vulkan::Swapchain mSwapchain;
        
        std::vector<Vulkan::FrameData> mFrameData;
        u32 mCurrentImageIndex = 0;
        u32 mCurrentFrame = 0;
        bool mResized = false;
    };
}  // namespace Swift
#pragma once
#include "SwiftEnums.hpp"

#include <Vulkan/VulkanStructs.hpp>

namespace Swift
{
    struct InitInfo
    {
        // Name for the app. Optional
        std::string appName{};
        // Name of the engine. Optional
        std::string engineName{};
        // Extent of the swapchain to be created. Mandatory
        glm::uvec2 extent{};
        // Native window handle. Mandatory
        // TODO: use std::variant to allow for linux support
        HWND hwnd{};
        // Enable imgui support. Optional
        bool enableImGui{};
        // Use for wider support of GPUs and possibly better performance. Optional
        bool bUsePipelines{};

        InitInfo& SetAppName(const std::string_view appName)
        {
            this->appName = appName;
            return *this;
        }
        InitInfo& SetEngineName(const std::string_view engineName)
        {
            this->engineName = engineName;
            return *this;
        }
        InitInfo& SetExtent(const glm::uvec2 extent)
        {
            this->extent = extent;
            return *this;
        }
        InitInfo& SetHwnd(const HWND hwnd)
        {
            this->hwnd = hwnd;
            return *this;
        }
        InitInfo& SetEnableImGui(const bool enableImGui)
        {
            this->enableImGui = enableImGui;
            return *this;
        }
        InitInfo& SetUsePipelines(const bool usePipelines)
        {
            this->bUsePipelines = usePipelines;
            return *this;
        }
    };

    struct DynamicInfo
    {
        glm::uvec2 extent{};

        DynamicInfo& SetExtent(const glm::uvec2 extent)
        {
            this->extent = extent;
            return *this;
        }
    };

    struct ShaderObject
    {
        u32 index;
        ShaderObject& SetIndex(const u32 index)
        {
            this->index = index;
            return *this;
        }
    };

    struct BufferObject
    {
        u32 index;
        u32 size;
        
        BufferObject& SetIndex(const u32 index)
        {
            this->index = index;
            return *this;
        }
        BufferObject& SetSize(const u32 size)
        {
            this->size = size;
            return *this;
        }
    };
    
    struct ImageObject
    {
        ImageType type;
        glm::uvec2 extent{};
        u32 index = 0;

        ImageObject& SetType(const ImageType type)
        {
            this->type = type;
            return *this;
        }
        ImageObject& SetExtent(const glm::uvec2 extent)
        {
            this->extent = extent;
            return *this;
        }
        ImageObject& SetIndex(const u32 index)
        {
            this->index = index;
            return *this;
        }
    };
}  // namespace Swift
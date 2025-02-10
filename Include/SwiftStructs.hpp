#pragma once

struct GLFWwindow;
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
        // TODO: add sdl support
        std::variant<GLFWwindow*> windowHandle{};

        // Use for wider support of GPUs and possibly better performance. Optional
        bool bUsePipelines{};

        // Use to prefer integrated graphics over dedicated ones (Dedicated graphics are
        // preferred by default)
        bool bPreferIntegratedGraphics{};

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
        InitInfo& SetWindowHandle(GLFWwindow* window)
        {
            this->windowHandle = window;
            return *this;
        }
        InitInfo& SetUsePipelines(const bool usePipelines)
        {
            this->bUsePipelines = usePipelines;
            return *this;
        }
        InitInfo& SetPreferIntegratedGraphics(const bool useIntegratedGraphics)
        {
            this->bPreferIntegratedGraphics = useIntegratedGraphics;
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

    inline u32 InvalidHandle = std::numeric_limits<u32>::max();
    using ShaderHandle = u32;
    using BufferHandle = u32;
    using ImageHandle = u32;
    using ThreadHandle = u32;

    struct BoundingSphere
    {
        glm::vec3 center{};
        float radius{};

        BoundingSphere& SetCenter(const glm::vec3& center)
        {
            this->center = center;
            return *this;
        }
        BoundingSphere& SetRadius(const float radius)
        {
            this->radius = radius;
            return *this;
        }
    };

    struct BoundingAABB
    {
        glm::vec3 center{};
        float padding{};
        glm::vec3 extents{};
        float padding2{};
    };

    struct Plane
    {
        glm::vec3 normal{};
        float distance{};
    };
    
    struct Frustum
    {
        Plane topFace;
        Plane bottomFace;

        Plane leftFace;
        Plane rightFace;

        Plane nearFace;
        Plane farFace;
    };
}  // namespace Swift
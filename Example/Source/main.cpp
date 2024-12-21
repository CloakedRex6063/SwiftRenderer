#include "Swift.hpp"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(1280, 720, "Example", nullptr, nullptr);

    const auto hwnd = glfwGetWin32Window(window);
    const auto initInfo = Swift::InitInfo()
                              .SetAppName("Example")
                              .SetEngineName("Swift Renderer")
                              .SetExtent(glm::uvec2(1280, 720))
                              .SetHwnd(hwnd);
    Swift::Renderer swiftRenderer;
    swiftRenderer.Init(initInfo);

    glm::uvec2 extent;
    glfwSetWindowUserPointer(window, &extent);
    glfwSetWindowSizeCallback(
        window,
        [](GLFWwindow* glfwWindow, const int width, const int height)
        {
            auto* windowExtent = static_cast<glm::uvec2*>(glfwGetWindowUserPointer(glfwWindow));
            windowExtent->x = width;
            windowExtent->y = height;
        });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        const auto dynamicInfo = Swift::DynamicInfo().SetExtent(extent);
        swiftRenderer.BeginFrame(dynamicInfo);
        swiftRenderer.Render();
        swiftRenderer.EndFrame(dynamicInfo);
    }

    swiftRenderer.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

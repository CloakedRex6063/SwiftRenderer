#include "Window.hpp"
#include "Input.hpp"
#ifdef SWIFT_IMGUI
#include "backends/imgui_impl_glfw.h"
#endif
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"


namespace
{
    struct WindowData
    {
        GLFWwindow* window;
        glm::uvec2 size;
        glm::vec2 cursorPos;
        std::unordered_map<Input::MouseButton, bool> mButtonsDown;
        std::unordered_map<Input::KeyboardKey, bool> mKeysDown;

        operator GLFWwindow*() const { return window; }
    } gWindow{};
} // namespace

namespace Window
{
    void Window::Init()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        gWindow.window = glfwCreateWindow(1280, 720, "Example", nullptr, nullptr);
        glfwGetWindowSize(gWindow, reinterpret_cast<int*>(&gWindow.size.x), reinterpret_cast<int*>(&gWindow.size.y));
        glfwSetWindowUserPointer(gWindow, &gWindow);
        glfwSetWindowSizeCallback(gWindow,
                                  [](GLFWwindow* glfwWindow, const int windowWidth, const int windowHeight)
                                  {
                                      auto* window = static_cast<WindowData*>(glfwGetWindowUserPointer(glfwWindow));
                                      window->size = glm::uvec2(windowWidth, windowHeight);
                                  });
    }

    void Window::Shutdown()
    {
        glfwDestroyWindow(gWindow);
        glfwTerminate();
    }
    
    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    glm::uvec2 Window::GetSize() { return gWindow.size; }
    
    GLFWwindow* Window::GetWindow()
    {
        return gWindow.window;
    }

    HWND Window::GetHandle() { return glfwGetWin32Window(gWindow); }

    bool Window::IsRunning() { return !glfwWindowShouldClose(gWindow); }
} // namespace Utility

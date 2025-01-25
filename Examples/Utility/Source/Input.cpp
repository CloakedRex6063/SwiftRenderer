#include "Input.hpp"

#include "GLFW/glfw3.h"

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
    };
    WindowData* internalWindow;

    void MouseButtonCallback(GLFWwindow* window,
                             int button,
                             const int action,
                             int mods)
    {
        const auto internalWindow = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        internalWindow->mButtonsDown[static_cast<Input::MouseButton>(button)] = action;
    }

    void MousePosCallback(GLFWwindow* window,
                          double xpos,
                          double ypos)
    {
        const auto internalWindow = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        internalWindow->cursorPos = glm::vec2(xpos, ypos);
    }

    void KeyCallback(GLFWwindow* window,
                     int key,
                     int scancode,
                     const int action,
                     int mods)
    {
        const auto internalWindow = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        internalWindow->mKeysDown[static_cast<Input::KeyboardKey>(key)] = action;
    }
}  // namespace

void Input::Init(GLFWwindow* window)
{
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, MousePosCallback);
    glfwSetKeyCallback(window, KeyCallback);
    internalWindow = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
}

bool Input::GetMouseButton(const MouseButton button)
{
    return internalWindow->mButtonsDown[button];
}

glm::vec2 Input::GetMousePosition()
{
    return internalWindow->cursorPos;
}
bool Input::GetKeyboardKey(const KeyboardKey button)
{
    return internalWindow->mKeysDown[button];
}

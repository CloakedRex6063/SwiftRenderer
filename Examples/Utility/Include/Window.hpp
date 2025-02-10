#pragma once

struct GLFWwindow;
namespace Window
{
    void Init();
    void Shutdown();

    void PollEvents();
    
    glm::uvec2 GetSize();
    GLFWwindow* GetWindow();
    bool IsRunning();
    void ImplImGUI();
} // namespace Utility::Window

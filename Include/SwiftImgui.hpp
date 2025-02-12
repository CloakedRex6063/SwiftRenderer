#pragma once
#include "variant"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "vk_mem_alloc.h"
#include "Swift.hpp"
#include "Vulkan/VulkanUtil.hpp"

namespace Swift::ImGUI
{
    inline void Init()
    {
        auto context = Swift::GetContext();
        auto [queue, index] = Swift::GetGraphicsQueue();
        auto initInfo = Swift::GetInitInfo();
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        auto format = VK_FORMAT_B8G8R8A8_UNORM;
        ImGui_ImplVulkan_InitInfo vulkanInitInfo{
            .Instance = context.instance,
            .PhysicalDevice = context.gpu,
            .Device = context.device,
            .QueueFamily = index,
            .Queue = queue,
            .MinImageCount = Vulkan::Util::GetSwapchainImageCount(context.gpu, context.surface),
            .ImageCount = Vulkan::Util::GetSwapchainImageCount(context.gpu, context.surface),
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .DescriptorPoolSize = 1000,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &format,
                .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            }};
        ImGui_ImplVulkan_Init(&vulkanInitInfo);
        ImGui_ImplVulkan_CreateFontsTexture();

        if (std::holds_alternative<GLFWwindow*>(initInfo.windowHandle))
        {
            ImGui_ImplGlfw_InitForVulkan(std::get<GLFWwindow*>(initInfo.windowHandle), true);
        }
    };

    inline void BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    inline void RenderImGUI()
    {
        Swift::BeginRendering();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(
            ImGui::GetDrawData(),
            Swift::GetGraphicsCommand().commandBuffer);
        Swift::EndRendering();
    }

    inline void ShowDebugStats()
    {
        const auto context = Swift::GetContext();
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(context.allocator, budgets);
        ImGui::Begin("Debug Statistics");
        ImGui::Text(
            "Memory Usage: %f GB",
            static_cast<float>(budgets[0].statistics.allocationBytes) /
                (1024.0f * 1024.0f * 1024.0f));
        ImGui::Text(
            "Memory Allocated: %f GB",
            static_cast<float>(budgets[0].usage) / (1024.0f * 1024.0f * 1024.0f));
        ImGui::Text(
            "Available GPU Memory: %f GB",
            static_cast<float>(budgets[0].budget) / (1024.0f * 1024.0f * 1024.0f));
        ImGui::End();
    }

    inline void EndFrame()
    {
        ImGui::EndFrame();
    }

    inline void Shutdown()
    {
        Swift::WaitIdle();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    };
};
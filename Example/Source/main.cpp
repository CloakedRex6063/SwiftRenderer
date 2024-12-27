#include "GLFW/glfw3.h"
#include "Parser.hpp"
#include "Swift.hpp"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/type_ptr.inl"

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

    glm::uvec2 extent;
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    extent = glm::uvec2(width, height);

    glfwSetWindowUserPointer(window, &extent);
    glfwSetWindowSizeCallback(
        window,
        [](GLFWwindow* glfwWindow, const int windowWidth, const int windowHeight)
        {
            auto* windowExtent = static_cast<glm::uvec2*>(glfwGetWindowUserPointer(glfwWindow));
            windowExtent->x = windowWidth;
            windowExtent->y = windowHeight;
        });

    std::vector<Model> models;
    Parser parser;
    parser.Init();
    const auto modelOpt =
        parser.LoadModelFromFile("../../../Example/Models/Helmet/DamagedHelmet.gltf");
    models.emplace_back(modelOpt.value());

    Swift::Init(initInfo);

    const auto ddsImage = Swift::CreateImageFromFile(
        "../../../Example/Models/Helmet/Default_albedo.dds",
        0,
        false);

    // struct ComputePushConstant
    // {
    //     int index;
    // } computePushConstant{};
    //
    // const auto computeShaderObject = Swift::CreateComputeShaderObject(
    //     "../Shaders/triangle.comp.spv",
    //     sizeof(ComputePushConstant));
    // const auto computeImage = Swift::CreateWriteableImage(initInfo.extent);
    // computePushConstant.index = computeImage.index;

    struct Camera
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
        float padding;
    } camera;

    camera.pos = glm::vec3(0, 0, 5.f);
    camera.view = glm::inverse(glm::translate(glm::mat4(1.0f), camera.pos));
    camera.proj = glm::perspective(glm::radians(60.f), 1280.f / 720.f, 0.1f, 1000.f);
    camera.proj[1][1] *= -1;

    const auto cameraBuffer = Swift::CreateBuffer(BufferType::eStorage, sizeof(Camera));
    Swift::UploadToBuffer(cameraBuffer, &camera, 0, sizeof(Camera));

    const auto transformBuffer =
        Swift::CreateBuffer(BufferType::eStorage, sizeof(glm::mat4) * 10000);
    Swift::UploadToBuffer(
        transformBuffer,
        models[0].transforms.data(),
        0,
        sizeof(glm::mat4) * models[0].transforms.size());

    const auto vertexSize = sizeof(Vertex) * models[0].vertices.size();
    const auto vertexPullBuffer = Swift::CreateBuffer(BufferType::eStorage, vertexSize);
    Swift::UploadToBuffer(vertexPullBuffer, models[0].vertices.data(), 0, vertexSize);

    auto materialSize = sizeof(Material) * models[0].materials.size();
    const auto materialPullBuffer =
        Swift::CreateBuffer(BufferType::eStorage, materialSize);
    Swift::UploadToBuffer(
        materialPullBuffer,
        models[0].materials.data(),
        0,
        materialSize);

    const auto indexSize = sizeof(u32) * models[0].indices.size();
    const auto indexBuffer = Swift::CreateBuffer(BufferType::eIndex, indexSize);
    Swift::UploadToBuffer(indexBuffer, models[0].indices.data(), 0, indexSize);

    struct ModelPushConstant
    {
        u64 vertexBufferAddress;
        u64 cameraBufferAddress;
        u64 transformBufferAddress;
        u64 materialBufferAddress;
        u64 transformIndex;
        u64 materialIndex;
    } modelPushConstant{};

    modelPushConstant.cameraBufferAddress = Swift::GetBufferAddress(cameraBuffer);
    modelPushConstant.transformBufferAddress = Swift::GetBufferAddress(transformBuffer);
    ;
    modelPushConstant.vertexBufferAddress = Swift::GetBufferAddress(vertexPullBuffer);
    modelPushConstant.materialBufferAddress = Swift::GetBufferAddress(materialPullBuffer);

    const auto modelShaderObject = Swift::CreateGraphicsShaderObject(
        "../Shaders/model.vert.spv",
        "../Shaders/model.frag.spv",
        sizeof(ModelPushConstant));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        const auto dynamicInfo = Swift::DynamicInfo().SetExtent(extent);
        Swift::BeginFrame(dynamicInfo);

        // Swift::BindShader(computeShaderObject);
        // Swift::PushConstant(computePushConstant);
        // Swift::DispatchCompute(std::ceil(extent.x / 32), std::ceil(extent.x / 32), 1);
        // Swift::BlitToSwapchain(computeImage, computeImage.extent);

        Swift::BeginRendering();

        Swift::BindShader(modelShaderObject);

        for (const auto& model : models)
        {
            Swift::BindIndexBuffer(indexBuffer);
            for (const auto& [vertexOffset, firstIndex, indexCount, materialIndex, transformIndex] :
                 model.meshes)
            {
                modelPushConstant.transformIndex = transformIndex;
                modelPushConstant.materialIndex = materialIndex;
                Swift::PushConstant(&modelPushConstant, sizeof(ModelPushConstant));
                Swift::DrawIndexed(indexCount, 1, firstIndex, vertexOffset, 0);
            }
        }

        Swift::EndRendering();

        Swift::EndFrame(dynamicInfo);
    }
    Swift::Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
}

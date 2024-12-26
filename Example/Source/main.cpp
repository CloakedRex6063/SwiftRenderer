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
        parser.LoadModelFromFile("../../../Example/Models/ToyCar.gltf");
    models.emplace_back(modelOpt.value());
    
    Swift::Renderer swiftRenderer;
    swiftRenderer.Init(initInfo);

    struct ComputePushConstant
    {
        int index;
    } computePushConstant{};

    const auto computeShaderObject = swiftRenderer.CreateComputeShaderObject(
        "../Shaders/triangle.comp.spv",
        sizeof(ComputePushConstant));
    const auto ddsImage = swiftRenderer.CreateImageFromFile("../../../Example/Models/Helmet/Default_albedo.dds");
    const auto computeImage = swiftRenderer.CreateWriteableImage(initInfo.extent);
    computePushConstant.index = computeImage.index;

    struct Camera
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 pos;
        float padding;
    } camera;

    camera.pos = glm::vec3(0, 0, 0.5f);
    camera.view = glm::inverse(glm::translate(glm::mat4(1.0f), camera.pos));
    camera.proj = glm::perspective(glm::radians(60.f), 1280.f / 720.f, 0.1f, 1000.f);
    camera.proj[1][1] *= -1;

    const auto cameraBuffer = swiftRenderer.CreateBuffer(BufferType::eStorage, sizeof(Camera));
    swiftRenderer.UploadToBuffer(cameraBuffer, &camera, 0, sizeof(Camera));

    const auto transformBuffer =
        swiftRenderer.CreateBuffer(BufferType::eStorage, sizeof(glm::mat4) * 10000);
    swiftRenderer.UploadToBuffer(
        transformBuffer,
        models[0].transforms.data(),
        0,
        sizeof(glm::mat4) * models[0].transforms.size());

    const auto vertexSize = sizeof(Vertex) * models[0].vertices.size();
    const auto vertexPullBuffer = swiftRenderer.CreateBuffer(BufferType::eStorage, vertexSize);
    swiftRenderer.UploadToBuffer(vertexPullBuffer, models[0].vertices.data(), 0, vertexSize);

    auto materialSize = sizeof(Material) * models[0].materials.size();
    const auto materialPullBuffer = swiftRenderer.CreateBuffer(BufferType::eStorage, materialSize);
    swiftRenderer.UploadToBuffer(materialPullBuffer, models[0].materials.data(), 0, materialSize);

    const auto indexSize = sizeof(u32) * models[0].indices.size();
    const auto indexBuffer = swiftRenderer.CreateBuffer(BufferType::eIndex, indexSize);
    swiftRenderer.UploadToBuffer(indexBuffer, models[0].indices.data(), 0, indexSize);

    struct ModelPushConstant
    {
        u64 vertexBufferAddress;
        u64 cameraBufferAddress;
        u64 transformBufferAddress;
        u64 materialBufferAddress;
        u64 transformIndex;
        u64 materialIndex;
    } modelPushConstant{};

    modelPushConstant.cameraBufferAddress = swiftRenderer.GetBufferAddress(cameraBuffer);
    modelPushConstant.transformBufferAddress = swiftRenderer.GetBufferAddress(transformBuffer);;
    modelPushConstant.vertexBufferAddress = swiftRenderer.GetBufferAddress(vertexPullBuffer);
    modelPushConstant.materialBufferAddress = swiftRenderer.GetBufferAddress(materialPullBuffer);

    const auto modelShaderObject = swiftRenderer.CreateGraphicsShaderObject(
        "../Shaders/model.vert.spv",
        "../Shaders/model.frag.spv",
        sizeof(ModelPushConstant));

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        const auto dynamicInfo = Swift::DynamicInfo().SetExtent(extent);
        swiftRenderer.BeginFrame(dynamicInfo);

        swiftRenderer.BindShader(computeShaderObject);
        swiftRenderer.PushConstant(computePushConstant);
        swiftRenderer.DispatchCompute(std::ceil(extent.x / 32), std::ceil(extent.x / 32), 1);

        swiftRenderer.BlitToSwapchain(computeImage, computeImage.extent);

        swiftRenderer.BeginRendering();

        swiftRenderer.BindShader(modelShaderObject);

        for (const auto& model : models)
        {
            swiftRenderer.BindIndexBuffer(indexBuffer);
            for (const auto& mesh : model.meshes)
            {
                modelPushConstant.transformIndex = mesh.transformIndex;
                modelPushConstant.materialIndex = mesh.materialIndex;
                swiftRenderer.PushConstant(modelPushConstant);
                swiftRenderer
                    .DrawIndexed(mesh.indexCount, 1, mesh.firstIndex, mesh.vertexOffset, 0);
            }
        }

        swiftRenderer.EndRendering();

        swiftRenderer.EndFrame(dynamicInfo);
    }
    swiftRenderer.Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
}

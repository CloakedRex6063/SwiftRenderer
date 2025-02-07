#include "Camera.hpp"
#include "Input.hpp"
#include "Parser.hpp"
#include "Structs.hpp"
#include "Swift.hpp"
#include "SwiftUtil.hpp"
#include "Window.hpp"
#include "future"
#include "imgui.h"

int main()
{
    // -----------------------Initialising API and required dependencies---------------------------

    Window::Init();
    Input::Init(Window::GetWindow());

    const auto swiftInitInfo = Swift::InitInfo()
                                   .SetAppName("Regular")
                                   .SetEngineName("Swift")
                                   .SetExtent(Window::GetSize())
                                   .SetHwnd(Window::GetHandle())
                                   .SetGlfwWindow(Window::GetWindow());
    Swift::Init(swiftInitInfo);
    Parser::Init();

    // --------------------------Initialising scene data and uploading to GPU-----------------------

    auto cameraData = Camera::Init(glm::vec3(0, 0, 2));
    const auto cameraBuffer =
        Swift::CreateBuffer(Swift::BufferType::eUniform, sizeof(CameraData), "Camera Buffer");
    Swift::UploadToBuffer(cameraBuffer, &cameraData, 0, sizeof(CameraData));

    Scene scene;
    Parser::LoadMeshes(scene, "../Resources/Helmet/DamagedHelmet.gltf");
    Scene cubeScene;
    const auto cubeIndex = Parser::LoadMeshes(cubeScene, "../Resources/Cube/Cube.gltf");
    const auto cubeVertexSize = cubeScene.vertices.size() * sizeof(Vertex);
    const auto cubeVertexBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, cubeVertexSize, "Vertex Buffer");
    Swift::UploadToBuffer(cubeVertexBuffer, cubeScene.vertices.data(), 0, cubeVertexSize);
    const auto cubeIndexSize = cubeScene.indices.size() * sizeof(u32);
    const auto cubeIndexBuffer =
        Swift::CreateBuffer(Swift::BufferType::eIndex, cubeIndexSize, "Index Buffer");
    Swift::UploadToBuffer(cubeIndexBuffer, cubeScene.indices.data(), 0, cubeIndexSize);

    const auto vertexSize = scene.vertices.size() * sizeof(Vertex);
    const auto vertexBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, vertexSize, "Vertex Buffer");
    Swift::UploadToBuffer(vertexBuffer, scene.vertices.data(), 0, vertexSize);

    const auto indexSize = scene.indices.size() * sizeof(u32);
    const auto indexBuffer =
        Swift::CreateBuffer(Swift::BufferType::eIndex, indexSize, "Index Buffer");
    Swift::UploadToBuffer(indexBuffer, scene.indices.data(), 0, indexSize);

    const auto materialSize = scene.materials.size() * sizeof(Material);
    const auto materialBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, materialSize, "Material Buffer");

    const auto transformSize = scene.transforms.size() * sizeof(glm::mat4);
    const auto transformBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, transformSize, "Transform Buffer");
    Swift::UploadToBuffer(transformBuffer, scene.transforms.data(), 0, transformSize);

    const auto boundingSize = scene.boundingSpheres.size() * sizeof(Swift::BoundingSphere);
    const auto boundingBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, boundingSize, "Bounding Sphere Buffer");
    Swift::UploadToBuffer(boundingBuffer, scene.boundingSpheres.data(), 0, boundingSize);

    std::vector<Light> lights;
    lights.emplace_back(glm::vec3(0, 0, 0), 1.f, glm::vec3(0, 0, 0), 0);
    const auto lightBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, sizeof(Light) * 10, "Light Buffer");
    Swift::UploadToBuffer(lightBuffer, lights.data(), 0, lights.size() * sizeof(Light));

    auto skybox = Swift::LoadCubemapFromFile("../Resources/HDRI/Footprint/Footprint.dds", "HDRI");
    auto irradiance = Swift::LoadCubemapFromFile(
        "../Resources/HDRI/Footprint/Footprint_Diffuse.dds",
        "Irradiance");
    auto specular = Swift::LoadCubemapFromFile(
        "../Resources/HDRI/Footprint/Footprint_Specular.dds",
        "Specular");
    auto lut =
        Swift::LoadImageFromFile("../Resources/HDRI/Footprint/Footprint_LUT.dds", 0, false, "Lut");

    scene.pushConstant = ModelPushConstant{
        .cameraBufferAddress = Swift::GetBufferAddress(cameraBuffer),
        .vertexBufferAddress = Swift::GetBufferAddress(vertexBuffer),
        .transformBufferAddress = Swift::GetBufferAddress(transformBuffer),
        .materialBufferAddress = Swift::GetBufferAddress(materialBuffer),
        .lightBufferAddress = Swift::GetBufferAddress(lightBuffer),
        .irradianceIndex = int(Swift::GetImageArrayIndex(irradiance)),
        .specularIndex = int(Swift::GetImageArrayIndex(specular)),
        .lutIndex = int(Swift::GetImageArrayIndex(lut)),
    };

    // -----------------------------Creating and uploading images in bulk--------------------------

    Swift::BeginTransfer();

    std::unordered_map<u32, int> textureIndices;
    for (const auto& [index, uri] : std::views::enumerate(scene.uris))
    {
        const auto image = Swift::LoadImageFromFileQueued(uri, 0, true, uri);
        textureIndices[index] = static_cast<int>(Swift::GetImageArrayIndex(image));
    }

    // Update the material texture indices so that we can index into the texture in the shader
    for (auto& material : scene.materials)
    {
        if (material.baseTextureIndex != -1)
        {
            material.baseTextureIndex = textureIndices.at(material.baseTextureIndex);
        }
        if (material.metallicRoughnessTextureIndex != -1)
        {
            material.metallicRoughnessTextureIndex =
                textureIndices.at(material.metallicRoughnessTextureIndex);
        }
        if (material.emissiveTextureIndex != -1)
        {
            material.emissiveTextureIndex = textureIndices.at(material.emissiveTextureIndex);
        }
        if (material.normalTextureIndex != -1)
        {
            material.normalTextureIndex = textureIndices.at(material.normalTextureIndex);
        }
        if (material.occlusionTextureIndex != -1)
        {
            material.occlusionTextureIndex = textureIndices.at(material.occlusionTextureIndex);
        }
    }
    Swift::UploadToBuffer(materialBuffer, scene.materials.data(), 0, materialSize);

    Swift::EndTransfer();

    // ------------------------------Creating all required shaders----------------------------------

    const auto skyboxShader = Swift::CreateGraphicsShader(
        "../Shaders/skybox.vert.spv",
        "../Shaders/skybox.frag.spv",
        "Skybox Shader");

    const auto graphicsShader = Swift::CreateGraphicsShader(
        "../Shaders/model.vert.spv",
        "../Shaders/model.frag.spv",
        "Model Shader");

    const auto indirectDrawShader = Swift::CreateGraphicsShader(
        "../Shaders/indirect_model.vert.spv",
        "../Shaders/indirect_model.frag.spv",
        "Indirect Model Shader");

    const auto indirectFillShader =
        Swift::CreateComputeShader("../Shaders/indirect.comp.spv", "Indirect Shader");
    const auto indirectCullShader =
        Swift::CreateComputeShader("../Shaders/indirectCull.comp.spv", "Cull Shader");

    // ---------------------Creating and uploading data for indirect drawing------------------------

    const u32 totalMeshes = scene.meshes.size();
    const u64 totalVertices = scene.vertices.size();
    const u64 totalTriangles = scene.indices.size() / 3;
    std::vector<PerDrawData> perDrawDatas;
    perDrawDatas.reserve(totalMeshes);
    std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
    indirectCommands.reserve(totalMeshes);
    for (auto& mesh : scene.meshes)
    {
        const auto drawIndirectCommand = vk::DrawIndexedIndirectCommand()
                                             .setFirstInstance(0)
                                             .setInstanceCount(1)
                                             .setFirstIndex(mesh.firstIndex)
                                             .setIndexCount(mesh.indexCount)
                                             .setVertexOffset(mesh.vertexOffset);
        indirectCommands.emplace_back(drawIndirectCommand);
        PerDrawData perDrawData{
            .materialIndex = mesh.materialIndex,
            .transformIndex = mesh.transformIndex,
        };
        perDrawDatas.emplace_back(perDrawData);
    }

    const auto indirectBuffer = Swift::CreateBuffer(
        Swift::BufferType::eIndirect,
        sizeof(vk::DrawIndexedIndirectCommand) * indirectCommands.size(),
        "Indirect Buffer");
    Swift::UploadToBuffer(
        indirectBuffer,
        indirectCommands.data(),
        0,
        sizeof(vk::DrawIndexedIndirectCommand) * indirectCommands.size());
    const auto perDrawBuffer = Swift::CreateBuffer(
        Swift::BufferType::eUniform,
        sizeof(PerDrawData) * perDrawDatas.size(),
        "Per Draw Buffer");
    Swift::UploadToBuffer(
        perDrawBuffer,
        perDrawDatas.data(),
        0,
        sizeof(PerDrawData) * perDrawDatas.size());

    // -------------------------------Filling push constants for shaders----------------------------

    IndirectDrawPushConstant indirectPC = {
        .perDrawBufferAddress = Swift::GetBufferAddress(perDrawBuffer),
        .cameraBufferAddress = Swift::GetBufferAddress(cameraBuffer),
        .lightBufferAddress = Swift::GetBufferAddress(lightBuffer),
        .vertexBufferAddress = Swift::GetBufferAddress(vertexBuffer),
        .transformBufferAddress = Swift::GetBufferAddress(transformBuffer),
        .materialBufferAddress = Swift::GetBufferAddress(materialBuffer),
        .lightCount = static_cast<int>(0),
        .irradianceIndex = static_cast<int>(Swift::GetImageArrayIndex(irradiance)),
        .specularIndex = static_cast<int>(Swift::GetImageArrayIndex(specular)),
        .lutIndex = static_cast<int>(Swift::GetImageArrayIndex(lut)),
    };

    const auto meshBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, sizeof(Mesh) * totalMeshes, "Mesh Buffer");
    Swift::UploadToBuffer(meshBuffer, scene.meshes.data(), 0, sizeof(Mesh) * totalMeshes);

    IndirectFillPushConstant indirectFillPC = {
        .indirectBuffer = Swift::GetBufferAddress(indirectBuffer),
        .meshBuffer = Swift::GetBufferAddress(meshBuffer),
        .meshCount = totalMeshes,
    };

    Swift::Frustum frustum;
    const auto frustumBuffer =
        Swift::CreateBuffer(Swift::BufferType::eUniform, sizeof(Swift::Frustum), "Frustum Buffer");

    const auto visibilityBuffer = Swift::CreateBuffer(
        Swift::BufferType::eStorage,
        sizeof(u32) * totalMeshes,
        "Visibility Buffer");

    IndirectFillCullPushConstant indirectCullPC = {
        .indirectBuffer = Swift::GetBufferAddress(indirectBuffer),
        .meshBuffer = Swift::GetBufferAddress(meshBuffer),
        .frustumBuffer = Swift::GetBufferAddress(frustumBuffer),
        .boundingBuffer = Swift::GetBufferAddress(boundingBuffer),
        .transformBuffer = Swift::GetBufferAddress(transformBuffer),
        .visBuffer = Swift::GetBufferAddress(visibilityBuffer),
        .meshCount = totalMeshes,
    };

    // --------------------------------------Camera Settings---------------------------------------

    float lookSensitivity = 5.f;
    float moveSensitivity = 2.f;
    float fov = 60.f;
    float nearClip = 0.01f;
    float frustumNear = 0.01f;
    float farClip = 1000.f;

    // -------------------------------------App Settings-------------------------------------------
    bool bGpuIndirect = false;
    bool bCpuFrustumCulling = false;
    bool bGpuFrustumCulling = false;
    float minLodDistance = 5.f;
    float maxLodDistance = 100.f;
    bool bShowLod = false;

    // For tracking delta-time
    std::chrono::high_resolution_clock::time_point lastTime =
        std::chrono::high_resolution_clock::now();

    std::future<std::unordered_map<Swift::ImageHandle, Swift::ImageHandle>> streamFuture;
    // -------------------------------------Game Loop----------------------------------------------
    while (Window::IsRunning())
    {
        Window::PollEvents();

        auto currentTime = std::chrono::high_resolution_clock::now();
        const auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        const auto currentWindowSize = Window::GetSize();
        const auto aspect =
            static_cast<float>(currentWindowSize.x) / static_cast<float>(currentWindowSize.y);
        const auto dynamicInfo = Swift::DynamicInfo().SetExtent(currentWindowSize);
        Camera::HandleKeyboard(cameraData, deltaTime, moveSensitivity);
        Camera::HandleMouse(cameraData, deltaTime, lookSensitivity);
        Camera::Update(cameraData, fov, currentWindowSize, nearClip, farClip);

        Swift::BeginFrame(dynamicInfo);

        Swift::UpdateSmallBuffer(cameraBuffer, 0, sizeof(CameraData), &cameraData);
        Swift::Visibility::UpdateFrustum(
            frustum,
            cameraData.view,
            cameraData.pos,
            frustumNear,
            farClip,
            glm::radians(fov),
            aspect);
        Swift::UpdateSmallBuffer(frustumBuffer, 0, sizeof(Swift::Frustum), &frustum);

        if (bGpuFrustumCulling)
        {
            Swift::BindShader(indirectCullShader);
            Swift::PushConstant(indirectCullPC);
            Swift::DispatchCompute(totalMeshes / 256 + 1, 1, 1);
        }

        else if (bGpuIndirect)
        {
            Swift::BindShader(indirectFillShader);
            Swift::PushConstant(indirectFillPC);
            Swift::DispatchCompute(totalMeshes / 256 + 1, 1, 1);
        }

        Swift::ClearSwapchainImage(glm::vec4(1, 0, 0, 0));
        Swift::BeginRendering();

        Swift::BindIndexBuffer(indexBuffer);

        if (bGpuIndirect || bGpuFrustumCulling)
        {
            Swift::BindShader(indirectDrawShader);
            Swift::PushConstant(indirectPC);
            Swift::DrawIndexedIndirect(
                indirectBuffer,
                0,
                totalMeshes,
                sizeof(vk::DrawIndexedIndirectCommand));
        }

        else
        {
            Swift::BindShader(graphicsShader);
            for (const auto& [index, mesh] : std::views::enumerate(scene.meshes))
            {
                auto& pushConstant = scene.pushConstant;
                pushConstant.transformIndex = mesh.transformIndex;
                pushConstant.materialIndex = mesh.materialIndex;

                if (bCpuFrustumCulling)
                {
                    const auto visible = Swift::Visibility::IsInFrustum(
                        frustum,
                        scene.boundingSpheres[index],
                        scene.transforms[mesh.transformIndex]);
                    if (!visible)
                        continue;
                }
                Swift::PushConstant(scene.pushConstant);
                Swift::DrawIndexed(mesh.indexCount, 1, mesh.firstIndex, mesh.vertexOffset, 0);
            }
        }

        SkyboxPushConstant skyboxPushConstant{
            .vertexBuffer = Swift::GetBufferAddress(cubeVertexBuffer),
            .cameraBuffer = Swift::GetBufferAddress(cameraBuffer),
            .cubemapIndex = Swift::GetImageArrayIndex(skybox),
        };

        Swift::BindShader(skyboxShader);
        Swift::BindIndexBuffer(cubeIndexBuffer);
        Swift::SetPolygonMode(Swift::PolygonMode::eFill);
        Swift::SetCullMode(Swift::CullMode::eFront);
        Swift::SetDepthCompareOp(Swift::DepthCompareOp::eLessOrEqual);
        Swift::PushConstant(skyboxPushConstant);

        const auto cube = cubeScene.meshes[cubeIndex.value()[0]];
        Swift::DrawIndexed(cube.indexCount, 1, cube.firstIndex, cube.vertexOffset, 0);

        Swift::EndRendering();

        Swift::ShowDebugStats();

        ImGui::Begin("App Settings");
        ImGui::Spacing();
        ImGui::Text("Camera Settings");
        ImGui::SliderFloat("Look Sensitivity", &lookSensitivity, 0.01f, 10.0f);
        ImGui::SliderFloat("Move Sensitivity", &moveSensitivity, 0.01f, 100.0f);
        ImGui::SliderFloat("FOV", &fov, 1.0f, 100.0f);
        ImGui::SliderFloat("Near Clip", &nearClip, 0.01f, 10.0f);
        ImGui::SliderFloat("Frustum Near Clip", &frustumNear, 0.01f, 10.0f);
        ImGui::SliderFloat("Far Clip", &farClip, 0.01f, 10000.0f);
        ImGui::Spacing();
        ImGui::Text("Draw Settings");
        ImGui::Checkbox("Gpu Indirect Drawing", &bGpuIndirect);
        ImGui::Checkbox("Cpu Culling", &bCpuFrustumCulling);
        ImGui::Checkbox("Gpu Culling", &bGpuFrustumCulling);
        ImGui::SliderFloat("Min LOD Distance", &minLodDistance, 0.01f, 100.0f);
        ImGui::SliderFloat("Max LOD Distance", &maxLodDistance, 0.01f, 1000.0f);
        ImGui::Checkbox("Show LOD", &bShowLod);

        ImGui::Text("Statistics");
        ImGui::Text("Total Vertices: %d", totalVertices);
        ImGui::Text("Total Triangles: %d", totalTriangles);
        ImGui::Text("Total Meshes: %d", totalMeshes);
        ImGui::Text("FPS: %f", 1.f / deltaTime);

        ImGui::End();
        Swift::RenderImGUI();
        Swift::EndFrame(dynamicInfo);
    }

    Swift::Shutdown();
    Window::Shutdown();
}

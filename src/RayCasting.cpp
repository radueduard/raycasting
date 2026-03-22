#include "RayCasting.h"

#include <commandBuffer.h>
#include <imgui.h>
#include <graphicsPipeline.h>
#include <shader.h>
#include <context.h>
#include <window.h>
#include <framebuffer.h>
#include <fstream>
#include "cubeMesh.h"
#include <descriptorSet.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <ranges>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.inl>

std::vector<glm::u8> loadRAWFile(const std::string& fileLocation, unsigned int x, unsigned int y, unsigned int z)
{
    if (!fileLocation.c_str())
    {
        std::cerr << fileLocation << "nu exista" << std::endl;
        throw std::runtime_error("File does not exist: " + fileLocation);
    }
    std::ifstream inFile(fileLocation.c_str(), std::ifstream::in | std::ifstream::binary);
    if (!inFile)
    {
        std::cerr << "Error openning file: " << fileLocation << std::endl;
        throw std::runtime_error("Failed to open file: " + fileLocation);
    }

    auto volumeData = std::vector<glm::u8>(x * y * z, 0);
    const auto sliceSize = x * y;

    for (int slice = 0; slice < z; slice++)
    {
        inFile.read(reinterpret_cast<char*>(volumeData.data() + slice * sliceSize), sliceSize);
        if (inFile.eof()) {
            const size_t bytecnt = inFile.gcount();
            std::cerr << "Unexpected end of file: " << fileLocation << ". Expected " << sliceSize << " bytes for slice " << slice << ", but got " << bytecnt << "." << std::endl;
            throw std::runtime_error("Unexpected end of file: " + fileLocation);
        } else if (inFile.fail()) {
            std::cerr << "Error reading file: " << fileLocation << " at slice " << slice << std::endl;
            throw std::runtime_error("Failed to read file: " + fileLocation);
        }
    }

    return volumeData;
}

std::unique_ptr<gfx::Image> createVolumeTexture(const std::string& fileLocation, unsigned int x, unsigned int y, unsigned int z) {

    auto volumeData = loadRAWFile(fileLocation, x, y, z);

    auto image = gfx::Image::Builder()
        .setType(gfx::Image::Type::e3D)
        .setFormat(gfx::Image::Format::eR8_UNORM)
        .setExtent({ x, y, z })
        .setUsage(gfx::Image::Usage::eSampled)
        .addUsage(gfx::Image::Usage::eTransferDst)
        .build();

    auto stagingBuffer = gfx::Buffer::Builder()
        .setSize(volumeData.size() * sizeof(glm::u8))
        .setUsage(gfx::Buffer::Usage::eTransferSrc)
        .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostVisible)
        .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostCoherent)
        .build();

    stagingBuffer->Map();
    stagingBuffer->Write(std::span { volumeData });
    stagingBuffer->Unmap();

    image->CopyFrom(*stagingBuffer, 0, 0);
    return image;
}

std::unique_ptr<gfx::Image> createTFTexture(const std::string& fileLocation) {
    std::ifstream inFile(fileLocation.c_str(), std::ifstream::in | std::ifstream::binary);
    if (!inFile)
    {
        std::cerr << "Error openning file: " << fileLocation << std::endl;
        throw std::runtime_error("Failed to open file: " + fileLocation);
    }

    constexpr int MAX_CNT = 256 * 4;
    std::vector<glm::u8> tff(MAX_CNT, 0);
    inFile.read(reinterpret_cast<char*>(tff.data()), MAX_CNT);
    if (inFile.eof()) {
        const size_t bytecnt = inFile.gcount();
        std::cout << "Functia de transfer: byte count = " << bytecnt << std::endl;
    } else if (inFile.fail()) {
        std::cerr << fileLocation << " nu s-a putut citi" << std::endl;
        throw std::runtime_error("Failed to open file: " + fileLocation);
    }

    auto tff1DTex = gfx::Image::Builder()
        .setType(gfx::Image::Type::e1D)
        .setFormat(gfx::Image::Format::eRGBA8_UNORM)
        .setExtent({ 256, 1, 1 })
        .setUsage(gfx::Image::Usage::eSampled)
        .addUsage(gfx::Image::Usage::eTransferDst)
        .build();

    auto stagingBuffer = gfx::Buffer::Builder()
        .setSize(tff.size() * sizeof(glm::u8))
        .setUsage(gfx::Buffer::Usage::eTransferSrc)
        .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostVisible)
        .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostCoherent)
        .build();

    for (int i = 0; i < 256; i++)
    {
        std::cout << "TF: " << static_cast<int>(tff[i * 4]) << " " << static_cast<int>(tff[i * 4 + 1]) << " " << static_cast<int>(tff[i * 4 + 2]) << " " << static_cast<int>(tff[i * 4 + 3]) << std::endl;
    }
    std::cout << "TF size: " << tff.size() << " bytes" << std::endl;

    stagingBuffer->Map();
    stagingBuffer->Write(std::span { tff });
    stagingBuffer->Unmap();

    tff1DTex->CopyFrom(*stagingBuffer, 0, 0);
    return tff1DTex;
}

void RayCasting::LoadVolumes()
{
    std::ifstream descriptionFile(ASSETS_PATH "readme.txt");
    if (!descriptionFile)
    {
        std::cerr << "Failed to open volume description file: " << ASSETS_PATH "readme.txt" << std::endl;
        throw std::runtime_error("Failed to open volume description file: " + std::string(ASSETS_PATH) + "readme.txt");
    }

    while (!descriptionFile.eof())
    {
        /* The description file is expected to have this structure:
         *
         * name.raw
         * w: x
         * h: y
         * d: z
         *
         * <again>
         */

        std::string name;
        std::getline(descriptionFile, name);
        if (name.empty()) {
            continue; // skip empty lines
        }
        name = name.substr(0, name.find_last_of('.')); // remove file extension for easier referencing
        std::string line;
        std::getline(descriptionFile, line);
        if (line.rfind("w:", 0) != 0) {
            std::cerr << "Expected width line starting with 'w:' but got: " << line << std::endl;
            throw std::runtime_error("Invalid volume description format in file: " + std::string(ASSETS_PATH) + "readme.txt");
        }
        unsigned int x = std::stoul(line.substr(2));
        std::getline(descriptionFile, line);
        if (line.rfind("h:", 0) != 0) {
            std::cerr << "Expected height line starting with 'h:' but got: " << line << std::endl;
            throw std::runtime_error("Invalid volume description format in file: " + std::string(ASSETS_PATH) + "readme.txt");
        }
        unsigned int y = std::stoul(line.substr(2));
        std::getline(descriptionFile, line);
        if (line.rfind("d:", 0) != 0) {
            std::cerr << "Expected depth line starting with 'd:' but got: " << line << std::endl;
            throw std::runtime_error("Invalid volume description format in file: " + std::string(ASSETS_PATH) + "readme.txt");
        }
        unsigned int z = std::stoul(line.substr(2));

        std::cout << "Loading volume: " << name << " with dimensions (" << x << ", " << y << ", " << z << ")" << std::endl;
        try {
            auto volumeTexture = createVolumeTexture(ASSETS_PATH + name + ".raw", x, y, z);
            std::cout << "Successfully loaded volume: " << name << std::endl;
            volumeData[name] = std::move(volumeTexture);
            volumeDataView[name] = gfx::ImageView::Builder(*volumeData[name])
                .setViewType(gfx::ImageView::Type::e3D)
                .build();
        } catch (const std::exception& e) {
            std::cerr << "Error loading volume " << name << ": " << e.what() << std::endl;
        }
    }

    currentVolumeName = volumeData.begin()->first; // select the first loaded volume by default
}

void RayCasting::Initialize()
{
    // TODO: setup resources
    cubeMesh = CubeMesh::Create();

    const auto backfaceVertex = gfx::Shader::Builder()
        .setPath(SHADERS_PATH "backFace.vert.glsl")
        .setStage(gfx::Shader::Stage::eVertex)
        .build();

    const auto backfaceFragment = gfx::Shader::Builder()
        .setPath(SHADERS_PATH "backFace.frag.glsl")
        .setStage(gfx::Shader::Stage::eFragment)
        .build();

    exitPoints = gfx::Image::Builder()
        .setIsPerFrame(true)
        .setFormat(gfx::Image::Format::eRGBA8_UNORM)
        .setExtent(gfx::Context::Window().getExtent())
        .setUsage(gfx::Image::Usage::eColorAttachment)
        .addUsage(gfx::Image::Usage::eSampled)
        .build();
    exitPointsView = gfx::ImageView::Builder(*exitPoints).build();
    exitPointsSampler = gfx::Sampler::Builder()
        .setMinFilter(gfx::Sampler::Filter::eLinear)
        .setMagFilter(gfx::Sampler::Filter::eLinear)
        .setAddressModeU(gfx::Sampler::AddressMode::eRepeat)
        .setAddressModeV(gfx::Sampler::AddressMode::eRepeat)
        .build();

    exitPointsFramebuffer = gfx::Framebuffer::Builder()
        .addColorAttachment(*exitPointsView, glm::vec4(0, 0, 0, 1))
        .build();

    calculateBackFacesPipeline = gfx::GraphicsPipeline::Builder()
        .setVertexShader<CubeMesh>(*backfaceVertex)
        .setFragmentShader(*backfaceFragment)
        .setFramebuffer(*exitPointsFramebuffer)
        .setRasterizationState(gfx::RasterizationState {
            .cullMode = gfx::CullMode::eFront,
            .frontFace = gfx::FrontFace::eCounterClockwise
        })
        .setDepthStencilState(gfx::DepthStencilState {
            .depthTestEnable = false,
            .stencilEnable = false,
        })
        .build();

    LoadVolumes();
    volumeSampler = gfx::Sampler::Builder()
        .setMinFilter(gfx::Sampler::Filter::eLinear)
        .setMagFilter(gfx::Sampler::Filter::eLinear)
        .setAddressModeU(gfx::Sampler::AddressMode::eRepeat)
        .setAddressModeV(gfx::Sampler::AddressMode::eRepeat)
        .setAddressModeW(gfx::Sampler::AddressMode::eRepeat)
        .build();

    tfTexture = createTFTexture(ASSETS_PATH "tff.dat");
    tfTextureView = gfx::ImageView::Builder(*tfTexture)
        .setViewType(gfx::ImageView::Type::e1D)
        .build();
    tfSampler = gfx::Sampler::Builder()
        .setMinFilter(gfx::Sampler::Filter::eLinear)
        .setMagFilter(gfx::Sampler::Filter::eLinear)
        .setAddressModeU(gfx::Sampler::AddressMode::eRepeat)
        .build();

    settingsBuffer = gfx::Buffer::Builder()
        .setSize(2 * sizeof(glm::vec4))
        .setUsage(gfx::Buffer::Usage::eUniform)
        .setMemoryProperties(gfx::Buffer::MemoryProperty::eHostVisible)
        .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostCoherent)
        .build();

    settingsBuffer->Map();
    settingsBuffer->Write(step);
    settingsBuffer->Write(backgroundColor, 4 * sizeof(float));
    settingsBuffer->Unmap();

    const glm::mat4 viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, -5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.f, 1.f, 0.f)
    );

    const glm::mat4 projectionMatrix = glm::perspective(
        glm::radians(45.f),
        static_cast<float>(gfx::Context::Window().getExtent().x) / static_cast<float>(gfx::Context::Window().getExtent().y),
        0.1f,
        100.f
    );

    const glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f)) *
                            glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));

    uniformBuffer = gfx::Buffer::Builder()
        .setSize(sizeof(glm::mat4) * 3) // model, view, projection
        .setUsage(gfx::Buffer::Usage::eUniform)
        .setMemoryProperties(gfx::Buffer::MemoryProperty::eHostVisible)
        .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostCoherent)
        .build();

    uniformBuffer->Map();
    uniformBuffer->Write(modelMatrix);
    uniformBuffer->Write(viewMatrix, sizeof(glm::mat4));
    uniformBuffer->Write(projectionMatrix, sizeof(glm::mat4) * 2);
    uniformBuffer->Unmap();


    transformationSet = gfx::DescriptorSet::Builder(calculateBackFacesPipeline->getSetLayout(0))
        .write(0, gfx::Descriptor(uniformBuffer.get()))
        .build();

    const auto raycastVertex = gfx::Shader::Builder()
        .setPath(SHADERS_PATH "raycast.vert.glsl")
        .setStage(gfx::Shader::Stage::eVertex)
        .build();

    const auto raycastFragment = gfx::Shader::Builder()
        .setPath(SHADERS_PATH "raycast.frag.glsl")
        .setStage(gfx::Shader::Stage::eFragment)
        .build();

    rayCastingPipeline = gfx::GraphicsPipeline::Builder()
        .setVertexShader<CubeMesh>(*raycastVertex)
        .setFragmentShader(*raycastFragment)
        .setRasterizationState(gfx::RasterizationState {
            .cullMode = gfx::CullMode::eBack,
            .frontFace = gfx::FrontFace::eCounterClockwise
        })
        .build();

    for (const auto& [name, view] : volumeDataView) {
        rayCastingSets[name] = gfx::DescriptorSet::Builder(rayCastingPipeline->getSetLayout(1))
            .write(0, gfx::Descriptor(view.get(), volumeSampler.get()))
            .write(1, gfx::Descriptor(tfTextureView.get(), tfSampler.get()))
            .write(2, gfx::Descriptor(exitPointsView.get(), exitPointsSampler.get()))
            .write(3, gfx::Descriptor(settingsBuffer.get()))
            .build();
    }
}

void RayCasting::Update()
{
    // TODO: per-frame logic
    if (!gfx::io::Input::isMouseButtonHeld(gfx::io::MouseButton::eRight)) {
        return;
    }

    bool changed = false;
    // move
    {
        glm::vec3 delta = { 0.0f, 0.0f, 0.0f };
        if (gfx::io::Input::isKeyHeld(gfx::io::Key::eW)) {
            delta.z += 1.f;
        }
        if (gfx::io::Input::isKeyHeld(gfx::io::Key::eS)) {
            delta.z -= 1.f;
        }
        if (gfx::io::Input::isKeyHeld(gfx::io::Key::eA)) {
            delta.x -= 1.f;
        }
        if (gfx::io::Input::isKeyHeld(gfx::io::Key::eD)) {
            delta.x += 1.f;
        }
        if (gfx::io::Input::isKeyHeld(gfx::io::Key::eQ)) {
            delta.y -= 1.f;
        }
        if (gfx::io::Input::isKeyHeld(gfx::io::Key::eE)) {
            delta.y += 1.f;
        }

        if (delta != glm::vec3(0.f)) {
            delta = glm::normalize(delta) * 5.f * gfx::io::Time::FrameTime();
            _cameraPosition += delta.x * _cameraRight + delta.y * glm::vec3(0.f, 1.f, 0.f) + delta.z * _cameraForward;
            changed = true;
        }
    }

    // rotate
    {
        if (const glm::vec2 mouseDelta = gfx::io::Input::getMousePositionDelta(); mouseDelta != glm::vec2(0.f))
        {
            constexpr float sensitivity = 0.2f;
            _cameraForward = glm::rotate(glm::mat4(1.f), -glm::radians(mouseDelta.x * sensitivity), glm::vec3(0.f, 1.f, 0.f)) *
                             glm::rotate(glm::mat4(1.f), -glm::radians(mouseDelta.y * sensitivity), _cameraRight) *
                             glm::vec4(_cameraForward, 0.f);

            _cameraRight = glm::normalize(glm::cross(_cameraForward, glm::vec3(0.f, 1.f, 0.f)));
            changed = true;
        }
    }

    if (!changed) {
        return;
    }

    const glm::mat4 viewMatrix = glm::lookAt(
        _cameraPosition,
        _cameraPosition + _cameraForward,
        glm::vec3(0.f, 1.f, 0.f)
    );

    uniformBuffer->Map();
    uniformBuffer->Write(viewMatrix, sizeof(glm::mat4));
    uniformBuffer->Unmap();
}

void RayCasting::Render(gfx::CommandBuffer& commandBuffer)
{
    // TODO: record render commands
    commandBuffer
        .BeginRendering(exitPointsFramebuffer.get())
        .SetViewport(0, 0, gfx::Context::Window().getExtent().x, gfx::Context::Window().getExtent().y)
        .SetScissor(0, 0 , gfx::Context::Window().getExtent().x, gfx::Context::Window().getExtent().y)
        .BindPipeline(calculateBackFacesPipeline.get())
        .BindDescriptorSet(0, transformationSet.get())
        .DrawMesh(cubeMesh.get(), 1, 0)
        .EndRendering()
        .BeginRendering()
        .SetViewport(0, 0, gfx::Context::Window().getExtent().x, gfx::Context::Window().getExtent().y)
        .SetScissor(0, 0 , gfx::Context::Window().getExtent().x, gfx::Context::Window().getExtent().y)
        .BindPipeline(rayCastingPipeline.get())
        .BindDescriptorSet(1, rayCastingSets[currentVolumeName].get())
        .DrawMesh(cubeMesh.get(), 1, 0)
        .EndRendering();
}

void RayCasting::RenderUI(ImGuiContext* context)
{
    ImGui::SetCurrentContext(context);
    // TODO: define ui for the scene
    ImGui::Begin("Volume Selection");
    for (const auto& name : volumeData | std::views::keys)
        if (ImGui::Selectable(name.c_str(), name == currentVolumeName))
            currentVolumeName = name;
    ImGui::End();
    ImGui::Separator();
    ImGui::Begin("Ray Casting Settings");
    bool changed = false;
    changed |= ImGui::SliderFloat("Step", &step, 0.0001f, 0.01f, "%.4f", ImGuiSliderFlags_Logarithmic);
    changed |= ImGui::ColorEdit4("Background Color", glm::value_ptr(backgroundColor));
    if (changed) {
        settingsBuffer->Map();
        settingsBuffer->Write(step);
        settingsBuffer->Write(backgroundColor, sizeof(float) * 4);
        settingsBuffer->Unmap();
    }
    ImGui::End();
}
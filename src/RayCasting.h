#pragma once

#include <commandBuffer.h>
#include <scene.h>
#include <image.h>
#include <imageView.h>
#include <buffer.h>
#include <sampler.h>
#include <framebuffer.h>
#include <graphicsPipeline.h>
#include <descriptorSet.h>
#include <gui.h>

class RayCasting final : public gfx::Scene
{
public:
    void LoadVolumes();
    void Initialize() override;
    void Update() override;
    void Render(gfx::CommandBuffer& commandBuffer) override;
    void RenderUI(ImGuiContext* context) override;

    std::unique_ptr<gfx::Image> tfTexture;
    std::unique_ptr<gfx::ImageView> tfTextureView;
    std::unique_ptr<gfx::Sampler> tfSampler;

    std::unique_ptr<gfx::Image> exitPoints;
    std::unique_ptr<gfx::ImageView> exitPointsView;
    std::unique_ptr<gfx::Sampler> exitPointsSampler;

    std::map<std::string, glm::uvec3> volumeSizes;
    std::map<std::string, std::unique_ptr<gfx::Image>> volumeData;
    std::map<std::string, std::unique_ptr<gfx::ImageView>> volumeDataView;
    std::unique_ptr<gfx::Sampler> volumeSampler;


    std::unique_ptr<gfx::Buffer> uniformBuffer;

    std::unique_ptr<gfx::Framebuffer> exitPointsFramebuffer;

    std::unique_ptr<gfx::GraphicsPipeline> calculateBackFacesPipeline;
    std::unique_ptr<gfx::GraphicsPipeline> rayCastingPipeline;

    std::unique_ptr<gfx::DescriptorSet> transformationSet;
    std::map<std::string, std::unique_ptr<gfx::DescriptorSet>> rayCastingSets;
    std::unique_ptr<gfx::Mesh> cubeMesh;

    std::string currentVolumeName;

    glm::vec3 _cameraPosition = glm::vec3(0.f, 0.f, -5.f);
    glm::vec3 _cameraForward = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 _cameraUp = glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 _cameraRight = glm::vec3(1.f, 0.f, 0.f);

    std::unique_ptr<gfx::Buffer> settingsBuffer;
    float step = 0.001f;
    glm::vec3 backgroundColor = glm::vec3(0.f, 0.f, 0.f);

    // gui info
    int currentZSlice = 0;
    std::unique_ptr<gfx::GUI_Image> slicePreview;
    std::unique_ptr<gfx::GUI_Image> gradientPreview;
};

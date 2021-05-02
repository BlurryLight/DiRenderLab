#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLwrapper/framebuffer.hh"
#include "GLwrapper/glsupport.hh"
#include "GLwrapper/program.hh"
#include "GLwrapper/shapes.hh"
#include "GLwrapper/texture.hh"
#include "GLwrapper/vertex_array.hh"
#include "GLwrapper/vertex_buffer.hh"
#include "utils/resource_path_searcher.h"
using DRL::Camera;
using DRL::Shader;

#include <iostream>


enum ShadowMode {
    kShadowMap = 0,
    kPCF = 1,
    kPCSS = 2,
};

class ShadowMapRender : public DRL::RenderBase {
public:
    DRL::ResourcePathSearcher resMgr;
    DRL::Program shader;
    DRL::Program simpleDepthShader;
    DRL::Program DepthMapShader;
    DRL::Program LightShader;
    DRL::VertexArray planeVAO;
    DRL::Texture2D woodTexture;
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    DRL::Texture2DPtr depthMap;
    DRL::Framebuffer depthMapFBO;

    float uBias = 0.05;
    float uPCFFilterSize = 5.0;
    float uPCSSBlockSize = 5.0;
    float uPCSSLightSize = 10.0;
    int current_mode = kShadowMap;
    glm::vec3 lightPos = {-2.0f, 4.0f, -1.0f};

    ShadowMapRender() = default;
    explicit ShadowMapRender(const BaseInfo &info) : DRL::RenderBase(info) {}
    void setup_states() override;
    void render() override;
    void renderScene(const DRL::Program &shader);
};
void ShadowMapRender::setup_states() {

    // IMGUI
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" / "shadowmap");
    resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");
    // build and compile shaders
    // -------------------------
    shader = DRL::make_program(
            resMgr.find_path("shadow_mapping.vert"),
            resMgr.find_path("shadow_mapping.frag"));
    simpleDepthShader = DRL::make_program(
            resMgr.find_path("shadow_mapping_depth.vert"),
            resMgr.find_path("shadow_mapping_depth.frag"));

    DepthMapShader = DRL::make_program(
            resMgr.find_path("debug_quad.vert"),
            resMgr.find_path("debug_quad_depth.frag"));

    LightShader = DRL::make_program(
            resMgr.find_path("shadow_mapping.vert"),
            resMgr.find_path("light.frag"));

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float planeVertices[] = {
            // positions            // normals         // texcoords
            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,

            25.0f, -0.5f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
            -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,
            25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f};
    // plane VAO

    auto planeVBO = std::make_shared<DRL::VertexBuffer>(planeVertices, sizeof(planeVertices), DRL::kStaticDraw);
    //    DRL::VertexArray planeVAO;
    planeVAO.lazy_bind_attrib(0, GL_FLOAT, 3, 0);
    planeVAO.lazy_bind_attrib(1, GL_FLOAT, 3, 3);
    planeVAO.lazy_bind_attrib(2, GL_FLOAT, 2, 6);
    planeVAO.update_bind(planeVBO, 0, 8, sizeof(GL_FLOAT));


    // load textures
    // -------------
    woodTexture = DRL::Texture2D(resMgr.find_path("wood.png"), false, true);

    // configure depth map FBO
    // -----------------------
    //    unsigned int depthMapFBO;
    //    glGenFramebuffers(1, &depthMapFBO);
    //    glCreateFramebuffers(1, &depthMapFBO);
    // create depth texture


    depthMap = std::make_shared<DRL::Texture2D>(SHADOW_WIDTH, SHADOW_HEIGHT, GL_DEPTH_COMPONENT32F, GL_FLOAT, nullptr);
    depthMap->set_wrap_s(GL_CLAMP_TO_EDGE);
    depthMap->set_wrap_t(GL_CLAMP_TO_EDGE);
    depthMap->bind();

    depthMapFBO = DRL::Framebuffer(GL_DEPTH_ATTACHMENT, depthMap, 0);
    depthMapFBO.set_draw_buffer(GL_NONE);
    depthMapFBO.set_read_buffer(GL_NONE);

    // shader configuration
    // --------------------
    shader.bind();
    shader.set_uniform("diffuseTexture", 0);
    shader.set_uniform("shadowMap", 1);


    // lighting info
    // -------------
}
void ShadowMapRender::render() {
    ImGui::NewFrame();
    {
        ImGui::Begin("Background Color", 0);// Create a window called "Hello,
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        const char *modes[] = {"ShadowMap", "PCF", "PCSS"};
        ImGui::Combo("ShadowModes", &current_mode, modes, IM_ARRAYSIZE(modes));
        if (ImGui::CollapsingHeader("ShadowMap")) {
            ImGui::SliderFloat("ShadowMap bias", &uBias, 0.0f, 1.0f, "%.3f");
        }

        if (ImGui::CollapsingHeader("PCF")) {
            ImGui::SliderFloat("PCF filter size", &uPCFFilterSize, 0.0f, 20.0f, "%.3f");
        }

        if (ImGui::CollapsingHeader("PCSS")) {
            ImGui::SliderFloat("PCSS block size", &uPCSSBlockSize, 0.0f, 20.0f, "%.3f");
            ImGui::SliderFloat("PCSS light size", &uPCSSLightSize, 0.0f, 100.0f, "%.3f");
        }
        ImGui::End();
    }
    ImGui::Render();

    // render
    // ------
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. render depth of scene to texture (from light's perspective)
    // --------------------------------------------------------------
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 50.0f;
    lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
    auto lightPosNew = lightPos + glm::vec3(
                                          5 * glm::sin(glfwGetTime()),
                                          20,
                                          5 * glm::sin(glfwGetTime()));
    lightView = glm::lookAt(lightPosNew, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;
    // render scene from light's point of view
    simpleDepthShader.bind();
    simpleDepthShader.set_uniform("lightSpaceMatrix", lightSpaceMatrix);

    {
        DRL::bind_guard<DRL::Framebuffer> fbo_gd(depthMapFBO);
        DRL::bind_guard<DRL::Texture2D> tex_gd(woodTexture);
        renderScene(simpleDepthShader);
    }

    // 2. render scene as normal using the generated depth/shadow map
    // --------------------------------------------------------------
    glViewport(0, 0, info_.width, info_.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.bind();
    glm::mat4 projection = glm::perspective(glm::radians(camera_->Zoom), (float) info_.width / (float) info_.height, 0.1f, 100.0f);
    glm::mat4 view = camera_->GetViewMatrix();
    shader.set_uniform("projection", projection);
    shader.set_uniform("view", view);
    // set light uniforms
    shader.set_uniform("viewPos", camera_->Position);
    shader.set_uniform("lightPos", lightPos);
    shader.set_uniform("lightSpaceMatrix", lightSpaceMatrix);
    shader.set_uniform("uBias", uBias);
    switch (current_mode) {
        case kShadowMap:
            break;
        case kPCF:
            shader.set_uniform("uPCFFilterSize", uPCFFilterSize);
            break;
        case kPCSS:
            shader.set_uniform("uPCSSBlockSize", uPCSSBlockSize);
            shader.set_uniform("uPCSSLightSize", uPCSSLightSize);
            break;
    }
    shader.set_uniform("uShadowMode", current_mode);
    woodTexture.set_slot(0);
    woodTexture.bind();
    depthMap->set_slot(1);
    depthMap->bind();
    renderScene(shader);
    depthMap->unbind();
    //render the light
    LightShader.bind();
    auto model = glm::mat4(1.0f);
    model = glm::translate(model, lightPosNew);
    model = glm::scale(model, glm::vec3(0.1f));
    LightShader.set_uniform("model", model);
    LightShader.set_uniform("projection", projection);
    LightShader.set_uniform("view", view);
    DRL::renderCube();

    // render Depth map to quad for visual debugging
    // ---------------------------------------------
    //        DepthMapShader.bind();
    //        DepthMapShader.set_uniform("near_plane", near_plane);
    //        DepthMapShader.set_uniform("far_plane", far_plane);
    //        depthMap.set_slot(0);
    //        depthMap.bind();
    //        glActiveTexture(GL_TEXTURE0);
    //        glBindTexture(GL_TEXTURE_2D, depthMap);
    //        DRL::renderQuad();
}

int main() {
    //spdlog init
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("log.txt", true));
    auto combined_logger = std::make_shared<spdlog::logger>("basic_logger", begin(sinks), end(sinks));
    //register it if you need to access it globally
    spdlog::register_logger(combined_logger);
    spdlog::set_default_logger(combined_logger);

    DRL::RenderBase::BaseInfo info;
    info.height = 900;
    info.width = 1600;
    ShadowMapRender rd(info);
    rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
    rd.loop();

    return 0;
}

// renders the 3D scene
// --------------------
void ShadowMapRender::renderScene(const DRL::Program &shader) {
    // floor
    glm::mat4 model = glm::mat4(1.0f);
    shader.set_uniform("model", model);
    {
        DRL::bind_guard gd(planeVAO);
        planeVAO.draw(GL_TRIANGLES, 0, 6);
    }
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.set_uniform("model", model);
    DRL::renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.set_uniform("model", model);
    DRL::renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.25));
    shader.set_uniform("model", model);
    //    renderCube();
    DRL::renderSphere();
}

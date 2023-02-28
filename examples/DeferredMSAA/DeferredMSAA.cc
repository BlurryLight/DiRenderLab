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

#include <iostream>

struct PointLight {
  glm::vec4 position;
  glm::vec4 color;

  float constant;
  float linear;
  float quadratic;
  float padding_;
};
std::vector<PointLight> lights;
class DeferredMSAARender : public DRL::RenderBase {
public:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program MRTShader;
  DRL::Program ShadingShader;
  DRL::Program DrawLightShader;

  std::unique_ptr<DRL::UniformBuffer> UniformBuffer;
  std::unique_ptr<DRL::Model> model_ptr;
  std::unique_ptr<DRL::Texture2D> woodTexture;

  DRL::Framebuffer GBufferFboMS;
  std::shared_ptr<DRL::Texture2DMS> gNormalMS;
  std::shared_ptr<DRL::Texture2DMS> gPositionMS;
  std::shared_ptr<DRL::Texture2DMS> gAlbedoMS;
  std::shared_ptr<DRL::Texture2DMS> gDepthMS;

  DeferredMSAARender() = default;
  explicit DeferredMSAARender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
  void renderScene(const DRL::Program &shader){};

  bool wireframe_ = false;
  glm::vec3 lightDir = glm::vec3(0, -1, 0);

  // 是否切换了MSAA选项
  bool bFrameBufferDirty = true;
  int MSAAMode = 0;
  int CurrentMSAA = 0;
};
void DeferredMSAARender::setup_states() {

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);

  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "DeferredMSAA");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "models" /
                  "sponza_compressed");
  // build and compile shaders
  // -------------------------

  MRTShader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                                resMgr.find_path("gBuffer.frag"));

  ShadingShader = DRL::make_program(resMgr.find_path("screen_quad.vert"),
                                    resMgr.find_path("Shading.frag"));

  DrawLightShader =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("DebugPointLight.frag"));
  woodTexture = std::make_unique<DRL::Texture2D>(resMgr.find_path("wood.png"),
                                                 5, false, true);
  woodTexture->set_wrap_s(GL_REPEAT);
  woodTexture->set_wrap_t(GL_REPEAT);
  woodTexture->generateMipmap();

  model_ptr = std::make_unique<DRL::Model>(
      resMgr.find_path("sponza.obj").string(), /*gamma*/ false, /*flip*/ false);

  for (auto &Mesh : model_ptr->meshes) {
    for (auto &texture : Mesh.textures_) {
      if (texture.type == "texture_diffuse") {
        texture.tex_ptr->set_sampler(DRL::TextureSampler::GetLinearRepeat());
      }
    }
  }

  int LightNum = 50;
  for (int j = 0; j < 5; j++) {
    for (int i = 0; i < LightNum / 5; i++) {
      PointLight light;
      light.color = glm::vec4(DRL::get_random_float(), DRL::get_random_float(),
                              DRL::get_random_float(), 0);
      light.linear = 1.f;
      light.constant = 1.0f;
      light.quadratic = 0.032f;
      light.position = glm::vec4(-10 + i * 2, 1, -5 + j * 2, 0);
      lights.push_back(light);
    }
  }
  UniformBuffer = std::make_unique<DRL::UniformBuffer>(
      lights.data(), sizeof(PointLight) * lights.size(),
      GL_DYNAMIC_STORAGE_BIT);
  UniformBuffer->set_slot(1);
}
void DeferredMSAARender::render() {

  if (bFrameBufferDirty) {
    CurrentMSAA = 2 * MSAAMode + 2;
    GBufferFboMS = DRL::Framebuffer();
    gNormalMS = std::make_shared<DRL::Texture2DMS>(info_.width, info_.height,
                                                   CurrentMSAA, GL_RGB16F);
    gPositionMS = std::make_shared<DRL::Texture2DMS>(info_.width, info_.height,
                                                     CurrentMSAA, GL_RGB16F);
    gAlbedoMS = std::make_shared<DRL::Texture2DMS>(info_.width, info_.height,
                                                   CurrentMSAA, GL_RGBA8);
    gDepthMS = std::make_shared<DRL::Texture2DMS>(
        info_.width, info_.height, CurrentMSAA, GL_DEPTH_COMPONENT32F);

    GBufferFboMS.attach_buffer(GL_COLOR_ATTACHMENT0, gNormalMS);
    GBufferFboMS.attach_buffer(GL_COLOR_ATTACHMENT1, gPositionMS);
    GBufferFboMS.attach_buffer(GL_COLOR_ATTACHMENT2, gAlbedoMS);
    GBufferFboMS.attach_buffer(GL_DEPTH_ATTACHMENT, gDepthMS);
    GBufferFboMS.set_viewport(info_.width, info_.height);
    GBufferFboMS.set_draw_buffer(
        {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2});
    GBufferFboMS.clear_color_ = glm::vec4(0);
    GBufferFboMS.clear_when_bind = true;
    bFrameBufferDirty = false;
  }

  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Checkbox("wireframe", &wireframe_);

    const char *modes[] = {"2X", "4X", "6X", "8X"};
    bFrameBufferDirty = ImGui::Combo("MSAA Level", (int *)&MSAAMode, modes,
                                     IM_ARRAYSIZE(modes));
    ImGui::SliderFloat3("lightDir", glm::value_ptr(lightDir), -1, 1);
    ImGui::End();
  }
  ImGui::Render();

  // render
  // ------
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, info_.width, info_.height);
  glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 1000.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  {
    DRL::bind_guard FboGuard(GBufferFboMS);
    {
      DRL::bind_guard ShaderGuard(MRTShader);
      MRTShader.set_uniform("projection", projection);
      MRTShader.set_uniform("view", view);
      MRTShader.set_uniform("model",
                            glm::translate(glm::mat4(1.0), glm::vec3(0, 1, 0)));
      {
        DRL::bind_guard TexGuard(this->woodTexture);
        DRL::renderCube();
      }
      MRTShader.set_uniform("model",
                            glm::scale(glm::mat4(1.0), glm::vec3(0.01)));
      model_ptr->Draw(MRTShader);
    }

    {
      DRL::bind_guard ShaderGuard(DrawLightShader);
      DrawLightShader.set_uniform("projection", projection);
      DrawLightShader.set_uniform("view", view);
      for (const auto &light : lights) {
        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(light.position));
        model = glm::scale(model, glm::vec3(0.1));
        DrawLightShader.set_uniform("model", model);
        DrawLightShader.set_uniform("lightColor", glm::vec3(light.color));
        DRL::renderSphere();
      }
    }
  }

  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    DRL::bind_guard ShaderGuard(ShadingShader, UniformBuffer);
    glUniformBlockBinding(
        ShadingShader, glGetUniformBlockIndex(ShadingShader, "PointLightBlock"),
        1);
    ShadingShader.set_uniform("lightDir", normalize(lightDir));
    gAlbedoMS->set_slot(0);
    gNormalMS->set_slot(1);
    gPositionMS->set_slot(2);

    DRL::bind_guard texGuard(gAlbedoMS, gNormalMS, gPositionMS);
    DRL::renderScreenQuad();
  }
}

int main() {
  // spdlog init
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
  sinks.push_back(
      std::make_shared<spdlog::sinks::basic_file_sink_st>("log.txt", true));
  auto combined_logger = std::make_shared<spdlog::logger>(
      "basic_logger", begin(sinks), end(sinks));
  // register it if you need to access it globally
  spdlog::register_logger(combined_logger);
  spdlog::set_default_logger(combined_logger);

  DRL::RenderBase::BaseInfo info;
  info.height = 900;
  info.width = 1600;
  info.msaa_sample_num = 0;
  DeferredMSAARender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.loop();

  return 0;
}

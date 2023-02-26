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

unsigned int uboExampleBlock;
#include <iostream>

enum GBufferMode : int {
  kNormal = 0,
  kPosition = 1,
  kAlbedo = 2,
};

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
  std::unique_ptr<DRL::Model> model_ptr;
  std::unique_ptr<DRL::Texture2D> woodTexture;

  DRL::Framebuffer GBufferFbo;
  std::shared_ptr<DRL::Texture2DMS> gNormal;
  std::shared_ptr<DRL::Texture2DMS> gPosition;
  std::shared_ptr<DRL::Texture2DMS> gAlbedo;
  std::shared_ptr<DRL::Texture2DMS> gDepth;

  DeferredMSAARender() = default;
  explicit DeferredMSAARender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
  void renderScene(const DRL::Program &shader){};

  bool wireframe_ = false;
  GBufferMode gBufferMode_ = kAlbedo;
  glm::vec3 lightDir = glm::vec3(0, -1, 0);
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
  glGenBuffers(1, &uboExampleBlock);
  glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLight) * lights.size(),
               lights.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  // set up fbo
  int mrt_msaa_samples = 4;
  gNormal = std::make_shared<DRL::Texture2DMS>(info_.width, info_.height,
                                               mrt_msaa_samples, GL_RGB16F);
  gPosition = std::make_shared<DRL::Texture2DMS>(info_.width, info_.height,
                                                 mrt_msaa_samples, GL_RGB16F);
  gAlbedo = std::make_shared<DRL::Texture2DMS>(info_.width, info_.height,
                                               mrt_msaa_samples, GL_RGBA8);
  gDepth = std::make_shared<DRL::Texture2DMS>(
      info_.width, info_.height, mrt_msaa_samples, GL_DEPTH_COMPONENT32F);

  GBufferFbo.attach_buffer(GL_COLOR_ATTACHMENT0, gNormal);
  GBufferFbo.attach_buffer(GL_COLOR_ATTACHMENT1, gPosition);
  GBufferFbo.attach_buffer(GL_COLOR_ATTACHMENT2, gAlbedo);
  GBufferFbo.attach_buffer(GL_DEPTH_ATTACHMENT, gDepth);
  GBufferFbo.set_viewport(info_.width, info_.height);
  GBufferFbo.set_draw_buffer(
      {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2});
  GBufferFbo.clear_color_ = glm::vec4(0);
  GBufferFbo.clear_when_bind = true;
}
void DeferredMSAARender::render() {
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Checkbox("wireframe", &wireframe_);

    const char *modes[] = {"Normal", "Position", "Albedo"};
    ImGui::Combo("ShadowModes", (int *)&gBufferMode_, modes,
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
    DRL::bind_guard FboGuard(GBufferFbo);
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
    DRL::bind_guard ShaderGuard(ShadingShader);
    glUniformBlockBinding(
        ShadingShader, glGetUniformBlockIndex(ShadingShader, "PointLightBlock"),
        1);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboExampleBlock);
    ShadingShader.set_uniform("lightDir", normalize(lightDir));
    gAlbedo->set_slot(0);
    gNormal->set_slot(1);
    gPosition->set_slot(2);

    DRL::bind_guard texGuard(gAlbedo, gNormal, gPosition);
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
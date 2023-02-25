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

class DeferredMSAARender : public DRL::RenderBase {
public:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  std::unique_ptr<DRL::Model> model_ptr;
  std::unique_ptr<DRL::Texture2D> woodTexture;

  DeferredMSAARender() = default;
  explicit DeferredMSAARender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
  void renderScene(const DRL::Program &shader){};

  bool wireframe_ = false;
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
  shader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                             resMgr.find_path("Shading.frag"));

  woodTexture = std::make_unique<DRL::Texture2D>(resMgr.find_path("wood.png"),
                                                 1, false, true);
  woodTexture->set_wrap_s(GL_REPEAT);
  woodTexture->set_wrap_t(GL_REPEAT);

  model_ptr = std::make_unique<DRL::Model>(
      resMgr.find_path("sponza.obj").string(), /*gamma*/ false, /*flip*/ false);

  GLuint samplerLinearRepeat = 0;
  glGenSamplers(1, &samplerLinearRepeat);
  glSamplerParameteri(samplerLinearRepeat, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glSamplerParameteri(samplerLinearRepeat, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glSamplerParameteri(samplerLinearRepeat, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(samplerLinearRepeat, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

  for(auto& Mesh : model_ptr->meshes)
  {
      for(auto & texture : Mesh.textures_)
      {
        if(texture.type == "texture_diffuse")
        {
          texture.tex_ptr->set_sampler(samplerLinearRepeat);
        }
      }
  }
}
void DeferredMSAARender::render() {
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Checkbox("wireframe", &wireframe_);
    ImGui::End();
  }
  ImGui::Render();

  // render
  // ------
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, info_.width, info_.height);
  glPolygonMode( GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL );

  shader.bind();
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 1000.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  shader.set_uniform("projection", projection);
  shader.set_uniform("view", view);
  shader.set_uniform("model", glm::mat4(1.0));
  {
    DRL::bind_guard gd(this->woodTexture);
    DRL::renderCube();
  }

  shader.set_uniform("model", glm::scale(glm::mat4(1.0),glm::vec3(0.01)));
  model_ptr->Draw(shader);
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
  info.msaa_sample_num = 4;
  DeferredMSAARender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.loop();

  return 0;
}


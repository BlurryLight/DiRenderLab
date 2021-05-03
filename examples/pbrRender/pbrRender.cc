//
// Created by zhong on 2021/5/3.
//

#include "pbrRender.hh"
#include "GLwrapper/shapes.hh"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
using namespace DRL;
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
  PbrRender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.loop();

  return 0;
}

void PbrRender::render() {

  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SliderFloat("metallic", &metallic_, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("roughness", &roughness_, 0.0f, 1.0f, "%.3f");
    ImGui::End();
  }
  ImGui::Render();

  shader.bind();
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  shader.set_uniform("projection", projection);
  shader.set_uniform("view", view);
  shader.set_uniform("model", glm::mat4(1.0));

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //    shader.bind();
  //    DRL::bind_guard<DRL::TextureCube> texture_gd(skyboxTexture);
  //  DRL::renderCube();
  DRL::renderSphere();
}
void PbrRender::setup_states() {
  glEnable(GL_DEPTH_TEST);
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  shader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                             resMgr.find_path("mvp_pos_normal_texture.frag"));
}

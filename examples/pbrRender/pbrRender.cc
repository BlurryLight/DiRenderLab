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
    ImGui::SliderFloat("metallic", &uniform_.metallic_, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("roughness", &uniform_.roughness_, 0.0f, 1.0f, "%.3f");
    ImGui::End();
  }
  ImGui::Render();
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    DRL::bind_guard gd(pbrShader);
    glm::mat4 view = camera_->GetViewMatrix();
    pbrShader.set_uniform("projection", uniform_.proj);
    pbrShader.set_uniform("view", view);
    pbrShader.set_uniform("model", glm::mat4(1.0));
    pbrShader.set_uniform("camPos", camera_->Position);
    pbrShader.set_uniform("metallic", uniform_.metallic_);
    pbrShader.set_uniform("roughness", uniform_.roughness_);
    pbrShader.set_uniform("lightPosition", uniform_.lightPos);
    pbrShader.set_uniform("lightColor", uniform_.lightColor);
    pbrShader.set_uniform("albedo", glm::vec3(0.5, 0, 0));
    pbrShader.set_uniform("ao", 1.0f);
    DRL::renderSphere();
  }

  {
    DRL::bind_guard gd(lightShader);
    glm::mat4 view = camera_->GetViewMatrix();
    lightShader.set_uniform("projection", uniform_.proj);
    lightShader.set_uniform("view", view);
    auto model = glm::translate(glm::mat4(1.0), uniform_.lightPos);
    lightShader.set_uniform("model", glm::scale(model, glm::vec3(0.1)));
    DRL::renderSphere();
  }
}
void PbrRender::setup_states() {
  glEnable(GL_DEPTH_TEST);
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "pbrRender");
  pbrShader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                                resMgr.find_path("pbr_direct.frag"));

  lightShader =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("light.frag"));
  uniform_.proj =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  uniform_.lightPos = glm::vec3(10.0f);
  uniform_.lightColor = glm::vec3(200.0f);
}

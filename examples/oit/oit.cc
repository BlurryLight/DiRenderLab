//
// Created by zhong on 2021/5/10.
//

#include "oit.hh"
#include "GLwrapper/shapes.hh"
#include "third_party/imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>
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
  OitRender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.loop();

  return 0;
}
void OitRender::setup_states() {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures" /
                  "oit");

  transparent_texture_ = std::make_shared<DRL::Texture2DARB>(
      resMgr.find_path("blending_transparent_window.png"), 1, false, false);
  pos_shader_ =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("mvp_pos_normal_texture.frag"));
  blend_shader_ =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("mvp_pos_n_t_sampler.frag"));
  transparent_texture_->make_resident();

  for (int i = 5; i >= 0; i--) {
    quad_pos_.emplace_back(0.0, 0.0, i);
  }
}
void OitRender::render() {

  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  ImGui::Render();

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(0, 0, -5));
  model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(1, 0, 0));
  auto view = camera_->GetViewMatrix();
  auto proj =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  {
    DRL::bind_guard gd(pos_shader_);
    pos_shader_.set_uniform("model", model);
    pos_shader_.set_uniform("view", view);
    pos_shader_.set_uniform("projection", proj);
    DRL::renderQuad();
  }

  {
    DRL::bind_guard gd(blend_shader_);
    blend_shader_.set_uniform("view", view);
    blend_shader_.set_uniform("projection", proj);
    blend_shader_.set_uniform("transparent_texture",
                              transparent_texture_->tex_handle_ARB());
    std::map<float, glm::vec3> sorted_quad;
    for (const auto &pos : quad_pos_) {
      float distance = glm::distance(pos, camera_->Position);
      sorted_quad[distance] = pos;
    }
    for (auto it = sorted_quad.rbegin(); it != sorted_quad.rend(); it++) {
      auto i_model = glm::translate(glm::mat4(1.0f), it->second);
      i_model =
          glm::rotate(i_model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
      blend_shader_.set_uniform("model", i_model);
      DRL::renderQuad();
    }
  }
}

//
// Created by zhong on 2021/5/1.
//

#include "instance.hh"
using namespace DRL;
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
void InstanceRender::setup_states() {
  glEnable(GL_DEPTH_TEST);
  auto root_path = decltype(resMgr)::root_path;
  resMgr.add_path(root_path / "resources" / "models" / "spot");
  resMgr.add_path(root_path / "resources" / "shaders" / "instance");
  spot_ptr = std::make_unique<DRL::Model>(
      resMgr.find_path("spot_triangulated_good.obj").string());
  AssertLog(bool(spot_ptr), "Load model failed!");

  shader = DRL::make_program(resMgr.find_path("asteroids.vert"),
                             resMgr.find_path("asteroids.frag"));

  InstanceShader =
      DRL::make_program(resMgr.find_path("asteroids_instance.vert"),
                        resMgr.find_path("asteroids.frag"));
  spotTexture =
      DRL::Texture2D(resMgr.find_path("spot_texture.png"), 1, false, false);
  spotTexture.generateMipmap();
  update_model_matrics();

  mScreenText = "Hello TextOverlay!";
  mScreenText.resize(512,0);
  mTextOverlay.drawText(mScreenText, 0, 2);
}
void InstanceRender::render() {

  
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::InputInt("Draw Numbers", &DrawNumbers);
    ImGui::SliderInt("Text Overlay Size", &mTextOverlay.mScale,1,5);
    if(ImGui::Button("Scroll"))
    {
      mTextOverlay.scroll(1);
    }
    const char *modes[] = {"Draw", "Instance"};
    ImGui::Combo("render Mode", &(mode_), modes, IM_ARRAYSIZE(modes));
    ImGui::End();
  }
  ImGui::Render();
  if (oldDrawNumbers != DrawNumbers) {
    oldDrawNumbers = DrawNumbers;
    update_model_matrics();
  }
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  spotTexture.bind();
  if (mode_ == kDraw) {
    shader.bind();
    shader.set_uniform("texture_diffuse1", 0);
    glm::mat4 projection = glm::perspective(
        glm::radians(camera_->Zoom), (float)info_.width / (float)info_.height,
        0.1f, 1000.0f);
    glm::mat4 view = camera_->GetViewMatrix();
    shader.set_uniform("projection", projection);
    shader.set_uniform("view", view);
    auto model_mat4 = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(),
                                  glm::vec3(0.0f, 0.1f, 0.0f));
    for (int i = 0; i < DrawNumbers; i++) {
      shader.set_uniform("model", model_mat4 * modelMatrics[i]);
      spot_ptr->Draw(shader);
    }
  } else if (mode_ == kInstance) {
    InstanceShader.bind();
    InstanceShader.set_uniform("texture_diffuse1", 0);
    modelMatricsSSBO->bind();
    glm::mat4 projection = glm::perspective(
        glm::radians(camera_->Zoom), (float)info_.width / (float)info_.height,
        0.1f, 1000.0f);
    glm::mat4 view = camera_->GetViewMatrix();
    InstanceShader.set_uniform("projection", projection);
    InstanceShader.set_uniform("view", view);
    InstanceShader.set_uniform("time_f", (float)glfwGetTime());
    for (int i = 0; i < spot_ptr->meshes.size(); i++) {
      glBindVertexArray(spot_ptr->meshes[i].vao_);
      glDrawElementsInstanced(GL_TRIANGLES, spot_ptr->meshes[i].indices_nums_,
                              GL_UNSIGNED_INT, nullptr, DrawNumbers);
      glBindVertexArray(0);
    }
    modelMatricsSSBO->unbind();
  }
  mTextOverlay.draw();
}
void InstanceRender::update_model_matrics() {

  modelMatrics.resize(DrawNumbers);
  float radius = 150.0;
  float offset = 25.0f;
  srand(1234); // fake random
  for (int i = 0; i < DrawNumbers; i++) {
    glm::mat4 model = glm::mat4(1.0f);
    // 1. translation: displace along circle with 'radius' in range [-offset,
    // offset]
    float angle = (float)i / (float)DrawNumbers * 360.0f;
    float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float x = sin(angle) * radius + displacement;
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float y = displacement * 0.4f; // keep height of asteroid field smaller
                                   // compared to width of x and z
    displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
    float z = cos(angle) * radius + displacement;
    model = glm::translate(model, glm::vec3(x, y, z));

    // 2. scale: Scale between 0.05 and 0.25f
    //        float scale = (rand() % 20) / 100.0f + 0.05;
    float scale = (rand() % 10) + 5.0;
    model = glm::scale(model, glm::vec3(scale));

    // 3. rotation: add random rotation around a (semi)randomly picked rotation
    // axis vector
    float rotAngle = (rand() % 360);
    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    // 4. now add to list of matrices
    //        modelMatrices[i] = model;
    modelMatrics[i] = model;
  }

  modelMatricsSSBO = std::make_unique<DRL::ShaderStorageBuffer>(modelMatrics.data(),DrawNumbers * sizeof(glm::mat4),GL_DYNAMIC_STORAGE_BIT);
  modelMatricsSSBO->set_slot(0);
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
  InstanceRender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 50.0, 305.0});
  rd.loop();

  return 0;
}

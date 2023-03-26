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
#include <array>
using DRL::Camera;
using DRL::Shader;

#include <iostream>

static constexpr int kDrawNums = 1024;
struct Light
{
  glm::vec3 dir;
  uint32_t : 32;
  glm::vec3 color;
  uint32_t : 32;
};
class DynamicIndexRenderer : public DRL::RenderBase {
public:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  std::vector<DRL::Texture2DARB> bindlessTextures;

  std::unique_ptr<DRL::UniformBuffer> LightUBO = nullptr;
  std::unique_ptr<DRL::ShaderStorageBuffer> modelMatricsSSBO = nullptr;

  DynamicIndexRenderer() = default;
  explicit DynamicIndexRenderer(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
  void renderScene(const DRL::Program &shader){};
};
void DynamicIndexRenderer::setup_states() {

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);

  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" / "dynamicIndex");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");
  // build and compile shaders
  // -------------------------
  shader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture_instance.vert"),
                             resMgr.find_path("mvp_pos_normal_texture_instance.frag"));

  // set matrix
  srand(1234);
  std::vector<glm::mat4>  matrices(kDrawNums,glm::mat4(1.0));
  for (int i = 0; i < kDrawNums; i++) {
    float x_offset = (2 * ((float)rand() / RAND_MAX) - 1.0) * 50;
    float y_offset = (2 * ((float)rand() / RAND_MAX) - 1.0) * 50;
    float z_offset = (float)rand() / RAND_MAX * -50;
    matrices[i] = glm::translate(glm::mat4(1.0), glm::vec3(x_offset, y_offset, z_offset));
  }
  modelMatricsSSBO = std::make_unique<DRL::ShaderStorageBuffer>(
      matrices.data(), kDrawNums * sizeof(glm::mat4), GL_DYNAMIC_STORAGE_BIT);
  modelMatricsSSBO->set_slot(0);

  LightUBO = std::make_unique<DRL::UniformBuffer>(nullptr,3 * sizeof(Light),GL_DYNAMIC_STORAGE_BIT);

  // textures

  for(int i = 0; i < 128 ;i++)
  {
    float r = (float)rand() / RAND_MAX;
    float g = (float)rand() / RAND_MAX;
    float b = (float)rand() / RAND_MAX;
    bindlessTextures.push_back(
      DRL::Texture2DARB::CreateDummyTexture(glm::vec4(r,g,b,1)));
  }
}
void DynamicIndexRenderer::render() {
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  ImGui::Render();

  // render
  // ------
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, info_.width, info_.height);

  float t = glfwGetTime();
  std::array<Light,3> lights{};
  lights[0].color = glm::vec3(0.5,0.4,0.4);
  lights[0].dir = glm::normalize(glm::vec3(1.0 * sin(t),1.0,-1.0));

  lights[1].color = glm::vec3(0.1,0.2,0.6);
  lights[1].dir = glm::normalize(glm::vec3(1.0 * cos(t),1.0,-1.0));

  lights[2].color = glm::vec3(0.1,0.7,0.2);
  lights[2].dir = glm::normalize(glm::vec3(1.0,1.0 * cos(t),-1.0));

  LightUBO->update_allocated_data(lights.data(), sizeof(lights), 0);
  LightUBO->bind();
  shader.bind();
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  shader.set_uniform("projection", projection);
  shader.set_uniform("view", view);
    
  for(int i = 0; i < 128;i++)
  {
    bindlessTextures[i].make_resident();
    shader.set_uniform("diffuseMaps[" + std::to_string(i) + "]",
                       bindlessTextures[i].tex_handle_ARB());
  }
  modelMatricsSSBO->bind();
  glBindVertexArray(DRL::GetCubeVAO());
  glDrawArraysInstanced(GL_TRIANGLES,0,36,kDrawNums);
  glBindVertexArray(0);
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
  DynamicIndexRenderer rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.camera_->SetTarget(glm::vec3(0));
  rd.loop();

  return 0;
}


// modified from opengl super bible 7th
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


static constexpr int kFlockSize = 16384;
struct flock_member
{
    glm::vec3 position;
    unsigned int : 32;
    glm::vec3 velocity;
    unsigned int : 32;
};

class CSFlockingRender : public DRL::RenderBase {
public:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  DRL::Program CsShader;
  std::shared_ptr<DRL::VertexBuffer> FlightBuffer = nullptr;
  DRL::VertexArray FlightVAO;
  DRL::ShaderStorageBuffer SSBOs[2];
  int mFrameIndex = 0;

  CSFlockingRender() = default;
  explicit CSFlockingRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
  void renderScene(const DRL::Program &shader){};
};
void CSFlockingRender::setup_states() {

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);

  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "CSFlocking");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");
  //
  static const glm::vec3 PaperFlight[] = {
      // Positions
      glm::vec3(-5.0f, 1.0f, 0.0f),
      glm::vec3(-1.0f, 1.5f, 0.0f),
      glm::vec3(-1.0f, 1.5f, 7.0f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, 10.0f),
      glm::vec3(1.0f, 1.5f, 0.0f),
      glm::vec3(1.0f, 1.5f, 7.0f),
      glm::vec3(5.0f, 1.0f, 0.0f),

      // Normals
      // the data is in local space
      glm::vec3(0.0f),
      glm::vec3(0.0f),
      glm::vec3(0.107f, -0.859f, 0.00f),
      glm::vec3(0.832f, 0.554f, 0.00f),
      glm::vec3(-0.59f, -0.395f, 0.00f),
      glm::vec3(-0.832f, 0.554f, 0.00f),
      glm::vec3(0.295f, -0.196f, 0.00f),
      glm::vec3(0.124f, 0.992f, 0.00f),
  };
  
  // build and compile shaders
  // -------------------------
  shader = DRL::make_program(resMgr.find_path("csflocking.vert"),
                             resMgr.find_path("csflocking.frag"));

  CsShader = DRL::make_cs_program(resMgr.find_path("csflocking.comp"));

  this->FlightBuffer = std::make_shared<DRL::VertexBuffer>(PaperFlight,sizeof(PaperFlight),0);
  FlightVAO.lazy_bind_attrib(0, GL_FLOAT, 3, 0);
  FlightVAO.lazy_bind_attrib(1, GL_FLOAT, 3, 8);
  FlightVAO.update_bind(FlightBuffer, 0, 1, sizeof(glm::vec3));

  SSBOs[0] =
      DRL::ShaderStorageBuffer(nullptr, sizeof(flock_member) * kFlockSize,
                               GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  SSBOs[1] =
      DRL::ShaderStorageBuffer(nullptr, sizeof(flock_member) * kFlockSize,
                               GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  SSBOs[0].set_slot(0);
  flock_member *ptr = reinterpret_cast<flock_member *>(
      glMapNamedBufferRange(SSBOs[0], 0, kFlockSize * sizeof(flock_member),
                            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

  // 只填充SSBO[0], SSBO[1]由compute shader填充
  for (int i = 0; i < kFlockSize; i++) {
    float x = DRL::get_random_float();
    float y = DRL::get_random_float();
    float z = DRL::get_random_float();
    ptr[i].position = (glm::vec3(x, y, z) - glm::vec3(0.5f)) * 100.0f;
    ptr[i].velocity = glm::vec3(0,0,0.001);
  }
  glUnmapNamedBuffer(SSBOs[0]);

}
void CSFlockingRender::render() {
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  ImGui::Render();

  float t = glfwGetTime();
  int curFrame = mFrameIndex;
  int nextFrame = (mFrameIndex + 1) % 2;
  // compute
  {

  SSBOs[curFrame].set_slot(0);
  SSBOs[nextFrame].set_slot(1);
  DRL::bind_guard gd(CsShader,SSBOs[0],SSBOs[1]);
    glm::vec3 goal = glm::vec3(sinf(t * 0.34f) * 10.0f, cosf(t * 0.29f) * 50.0,
                              sinf(t * 0.12f) * cosf(t * 0.5f) * 60);
    CsShader.set_uniform("goal", goal);
   glDispatchCompute(kFlockSize / 256, 1, 1);
  }

  // render
  // ------
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, info_.width, info_.height);
  glDisable(GL_CULL_FACE);

  shader.bind();
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 1000.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  shader.set_uniform("projection", projection);
  shader.set_uniform("view", view);
  // shader.set_uniform("model", glm::mat4(1.0));
  {
    SSBOs[curFrame].set_slot(0);
    DRL::bind_guard gd(FlightVAO, SSBOs[curFrame]);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 8, kFlockSize);
  }
  mFrameIndex = nextFrame;
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
  CSFlockingRender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{100.0, 0.0, 400.0});
  rd.camera_->Front = normalize(glm::vec3(0) - rd.camera_->Position);
  rd.loop();

  return 0;
}


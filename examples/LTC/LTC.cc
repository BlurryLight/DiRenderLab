#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

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
using DRL::OffsetOrientation;

#include <iostream>

static glm::vec3 LightEulerAngles = glm::vec3(90, 0, 0);

bool gUseQuat = false;
bool gQuatRight = true;

struct uniform_block {
  glm::vec3 lightPos{0, 1, 0};
  glm::quat lightRotation;
  glm::vec3 lightAngle{0};
  glm::vec3 lightScale{1.0};
  bool twoSided = true;
  float roughness = 0.3;
};

class LTCRender : public DRL::RenderBase {
public:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program LTCShader;
  DRL::Program LightShader;
  DRL::VertexArray planeVAO;
  DRL::Texture2D woodTexture;
  DRL::Texture2D ltc1tex, ltc2tex;
  uniform_block uniforms_;
  glm::vec3 lightCenter_;
  std::vector<glm::vec3> lightCorners_;

  LTCRender() = default;
  explicit LTCRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
  void renderScene(const DRL::Program &shader);
  virtual void processInput() override;
};
void LTCRender::setup_states() {

  // IMGUI
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);

  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "LTC");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures" /
                  "ltc");
  // build and compile shaders
  // -------------------------
  LTCShader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                                resMgr.find_path("ltc.frag"));

  LightShader =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("light.frag"));

  lightCenter_ = glm::vec3(0.0, -0.5, 0.0);
  lightCorners_.push_back(glm::vec3(1.0, -0.5, 1.0));
  lightCorners_.push_back(glm::vec3(-1.0, -0.5, 1.0));
  lightCorners_.push_back(glm::vec3(-1.0, -0.5, -1.0));
  lightCorners_.push_back(glm::vec3(1.0, -0.5, -1.0));
  // set up vertex data (and buffer(s)) and configure vertex attributes
  // ------------------------------------------------------------------
  float planeVertices[] = {
      // positions            // normals         // texcoords
      25.0f,  -0.5f, 25.0f,  0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
      -25.0f, -0.5f, 25.0f,  0.0f, 1.0f, 0.0f, 0.0f,  0.0f,
      -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f,  25.0f,

      25.0f,  -0.5f, 25.0f,  0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
      -25.0f, -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f,  25.0f,
      25.0f,  -0.5f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f};
  // plane VAO

  auto planeVBO = std::make_shared<DRL::VertexBuffer>(
      planeVertices, sizeof(planeVertices), GL_DYNAMIC_STORAGE_BIT);
  //    DRL::VertexArray planeVAO;
  planeVAO.lazy_bind_attrib(0, GL_FLOAT, 3, 0);
  planeVAO.lazy_bind_attrib(1, GL_FLOAT, 3, 3);
  planeVAO.lazy_bind_attrib(2, GL_FLOAT, 2, 6);
  planeVAO.update_bind(planeVBO, 0, 8, sizeof(float));

  // load textures
  // -------------
  woodTexture = DRL::Texture2D(resMgr.find_path("wood.png"), 1, false, true);
  woodTexture.set_wrap_s(GL_REPEAT);
  woodTexture.set_wrap_t(GL_REPEAT);

  ltc1tex = DRL::Texture2D(resMgr.find_path("ltc_1.png"), 1, false, false);
  ltc2tex = DRL::Texture2D(resMgr.find_path("ltc_2.png"), 1, false, false);

  // shader configuration
  // --------------------
  LTCShader.bind();
  LTCShader.set_uniform("diffuseTexture", 0);
  LTCShader.set_uniform("ltc_1", 1);
  LTCShader.set_uniform("ltc_2", 2);

  // lighting info
  // -------------
  uniforms_.lightRotation =
      glm::quat(glm::radians(glm::vec3(90.0f, 0.0f, 0.0f)));
}


void LTCRender::render() {

  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SliderFloat3("LightPos", glm::value_ptr(uniforms_.lightPos), -10.0f,
                        10.0f, "%.3f");
    ImGui::SliderFloat3("LightQuatAngles", glm::value_ptr(uniforms_.lightAngle),
                        0, 180.0f, "%.3f");
    ImGui::SliderFloat4("LightQuatValues",
                        glm::value_ptr(uniforms_.lightRotation), 0, 1.0f,
                        "%.3f");

    ImGui::SliderFloat3("LightEulerAngles", glm::value_ptr(LightEulerAngles), 0,
                        180.0f, "%.3f");
    ImGui::SliderFloat3("LightScale", glm::value_ptr(uniforms_.lightScale), 0.1,
                        2.0f, "%.3f");
    ImGui::Checkbox("TwoSided", &uniforms_.twoSided);
    ImGui::Checkbox("Use Quat", &gUseQuat);
    ImGui::Checkbox("Quat Right",&gQuatRight);
    if(ImGui::Button("Reset Quat / Euler"))
    {
      uniforms_.lightRotation =
      glm::quat(glm::radians(glm::vec3(90.0f, 0.0f, 0.0f)));
      LightEulerAngles = glm::vec3(90, 0, 0);
    }
    ImGui::SliderFloat("roughness", &uniforms_.roughness, 0.01, 1.0, "%.3f");
    ImGui::End();
  }
  ImGui::Render();

  // render
  // ------
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  glm::mat4 view = camera_->GetViewMatrix();

  // render the light
  LightShader.bind();
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, uniforms_.lightPos);

  // model = model * glm::eulerAngleYXZ(radians.y,radians.x,radians.z); //gimbal
  // block when x == 90.0
  uniforms_.lightAngle =
      glm::degrees(glm::eulerAngles(uniforms_.lightRotation));

  if(gUseQuat)
  {
    model = model * glm::mat4_cast(uniforms_.lightRotation); // quat version
  }
  else
  {
    auto radians = glm::radians(LightEulerAngles);
    glm::mat4 rotationEuler = glm::eulerAngleYXZ(radians.y, radians.x, radians.z);
    model = model * rotationEuler; // euler version
  }
  model = glm::scale(model, uniforms_.lightScale);
  LightShader.set_uniform("model", model);
  LightShader.set_uniform("projection", projection);
  LightShader.set_uniform("view", view);
  DRL::renderQuad();

  LTCShader.bind();
  // glm::vec3 lightCenter = glm::vec3(model * glm::vec4(lightCenter_, 1.0));
  //  LTCShader.set_uniform("lightCenter", lightCenter);
  LTCShader.set_uniform("lightPoints[0]",
                        glm::vec3(model * glm::vec4(lightCorners_[0], 1.0)));
  LTCShader.set_uniform("lightPoints[1]",
                        glm::vec3(model * glm::vec4(lightCorners_[1], 1.0)));
  LTCShader.set_uniform("lightPoints[2]",
                        glm::vec3(model * glm::vec4(lightCorners_[2], 1.0)));
  LTCShader.set_uniform("lightPoints[3]",
                        glm::vec3(model * glm::vec4(lightCorners_[3], 1.0)));
  LTCShader.set_uniform("camPos", camera_->Position);

  LTCShader.set_uniform("projection", projection);
  LTCShader.set_uniform("view", view);
  LTCShader.set_uniform("model", glm::mat4(1.0));
  LTCShader.set_uniform("twoSided", uniforms_.twoSided);
  LTCShader.set_uniform("roughness", uniforms_.roughness);
  woodTexture.set_slot(0);
  woodTexture.bind();
  ltc1tex.set_slot(1);
  ltc1tex.bind();
  ltc2tex.set_slot(2);
  ltc2tex.bind();
  renderScene(LTCShader);
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
  LTCRender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.loop();

  return 0;
}

// renders the 3D scene
// --------------------
void LTCRender::renderScene(const DRL::Program &shader) {
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
}

inline void LTCRender::processInput() {
  DRL::RenderBase::processInput();
  if (!gUseQuat) {
    if (glfwGetKey(window_, GLFW_KEY_J) == GLFW_PRESS)
      LightEulerAngles.x -= 0.1;
    if (glfwGetKey(window_, GLFW_KEY_U) == GLFW_PRESS)
      LightEulerAngles.x += 0.1;

    if (glfwGetKey(window_, GLFW_KEY_K) == GLFW_PRESS)
      LightEulerAngles.y -= 0.1;
    if (glfwGetKey(window_, GLFW_KEY_I) == GLFW_PRESS)
      LightEulerAngles.y += 0.1;

    if (glfwGetKey(window_, GLFW_KEY_L) == GLFW_PRESS)
      LightEulerAngles.z -= 0.1;
    if (glfwGetKey(window_, GLFW_KEY_O) == GLFW_PRESS)
      LightEulerAngles.z += 0.1;
  } else {
    if (glfwGetKey(window_, GLFW_KEY_J) == GLFW_PRESS)
      uniforms_.lightRotation = OffsetOrientation(
          glm::vec3(1.0f, 0.0f, 0.0f), -0.1, uniforms_.lightRotation,gQuatRight);
    if (glfwGetKey(window_, GLFW_KEY_U) == GLFW_PRESS)
      uniforms_.lightRotation = OffsetOrientation(glm::vec3(1.0f, 0.0f, 0.0f),
                                                  0.1, uniforms_.lightRotation,gQuatRight);

    if (glfwGetKey(window_, GLFW_KEY_K) == GLFW_PRESS)
      uniforms_.lightRotation = OffsetOrientation(
          glm::vec3(0.0f, 1.0f, 0.0f), -0.1, uniforms_.lightRotation,gQuatRight);
    if (glfwGetKey(window_, GLFW_KEY_I) == GLFW_PRESS)
      uniforms_.lightRotation = OffsetOrientation(glm::vec3(0.0f, 1.0f, 0.0f),
                                                  0.1, uniforms_.lightRotation,gQuatRight);

    if (glfwGetKey(window_, GLFW_KEY_L) == GLFW_PRESS)
      uniforms_.lightRotation = OffsetOrientation(
          glm::vec3(0.0f, 0.0f, 1.0f), -0.1, uniforms_.lightRotation,gQuatRight);
    if (glfwGetKey(window_, GLFW_KEY_O) == GLFW_PRESS)
      uniforms_.lightRotation = OffsetOrientation(glm::vec3(0.0f, 0.0f, 1.0f),
                                                  0.1, uniforms_.lightRotation,gQuatRight);
  }
}

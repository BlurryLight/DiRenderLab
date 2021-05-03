//
// Created by zhong on 2021/4/30.
//

#include "skybox.hh"
#include "GLwrapper/texture.hh"
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
  SkyboxRender rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 3.0});
  rd.loop();

  return 0;
}
void SkyboxRender::render() {

  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    const char *modes[] = {"Mode0", "Mode1"};
    ImGui::Combo("skybox Mode", &skyboxMode, modes, IM_ARRAYSIZE(modes));
    ImGui::End();
  }
  ImGui::Render();

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //    shader.bind();
  //    DRL::bind_guard<DRL::TextureCube> texture_gd(skyboxTexture);
  DRL::bind_guard gd(shader);
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  shader.set_uniform("projection", projection);
  shader.set_uniform("view", view);
  shader.set_uniform("model", glm::mat4(1.0));
  DRL::renderCube();
  {
    DRL::bind_guard<DRL::VertexArray> gd(skyboxVAO);
    glDepthFunc(GL_LEQUAL);
    if (skyboxMode == 0) {
      skyboxShader.bind();
      skyboxShader.set_uniform("projection", projection);
      skyboxShader.set_uniform("view", glm::mat4(glm::mat3(view)));
      glDrawArrays(GL_TRIANGLES, 0, 36);
      skyboxShader.unbind();
    } else {
      skyboxShader2.bind();
      skyboxShader2.set_uniform("view", view);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      skyboxShader2.unbind();
    }
    glDepthFunc(GL_LESS);
  }
}
void SkyboxRender::setup_states() {
  glEnable(GL_DEPTH_TEST);
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "skybox");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures" /
                  "skyboxlake");
  shader = DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                             resMgr.find_path("cube_reflect.frag"));
  skyboxShader = DRL::make_program(resMgr.find_path("skybox.vert"),
                                   resMgr.find_path("skybox.frag"));
  skyboxShader2 = DRL::make_program(resMgr.find_path("skybox_2.vert"),
                                    resMgr.find_path("skybox.frag"));
  // clang-format off
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };
  // clang-format on
  auto skyboxVBO = std::make_shared<DRL::VertexBuffer>(
      skyboxVertices, sizeof skyboxVertices, DRL::kStaticDraw);
  skyboxVAO.lazy_bind_attrib(0, GL_FLOAT, 3, 0);
  // a simple test: bind vbo to vao's second slot
  skyboxVAO.update_bind_slot(skyboxVBO, 1, 0, 3, sizeof(GL_FLOAT));
  std::vector<fs::path> faces{
      resMgr.find_path("right.jpg"), resMgr.find_path("left.jpg"),
      resMgr.find_path("top.jpg"),   resMgr.find_path("bottom.jpg"),
      resMgr.find_path("front.jpg"), resMgr.find_path("back.jpg"),
  };
  skyboxTexture = DRL::TextureCube(faces, false, false);
  auto handle = glGetTextureHandleARB(skyboxTexture);
  glMakeTextureHandleResidentARB(handle);
  GLint loc = glGetUniformLocation(shader, "skybox");
  glProgramUniformHandleui64ARB(shader, loc, handle);
  loc = glGetUniformLocation(skyboxShader, "skybox");
  glProgramUniformHandleui64ARB(skyboxShader, loc, handle);
  loc = glGetUniformLocation(skyboxShader2, "skybox");
  glProgramUniformHandleui64ARB(skyboxShader2, loc, handle);
}

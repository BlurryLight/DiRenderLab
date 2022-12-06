#include "PboBench.hh"
using namespace DRL;
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"

GLuint PBOBuffers[2] = {0, 0};

void PBOBench::setup_states() {
  glEnable(GL_DEPTH_TEST);
  auto root_path = decltype(resMgr)::root_path;
  resMgr.add_path(root_path / "resources" / "models" / "spot");
  resMgr.add_path(root_path / "resources" / "shaders" / "instance");
  spot_ptr = std::make_unique<DRL::Model>(
      resMgr.find_path("spot_triangulated_good.obj").string());
  AssertLog(bool(spot_ptr), "Load model failed!");

  shader = DRL::make_program(resMgr.find_path("asteroids.vert"),
                             resMgr.find_path("asteroids.frag"));

  update_model_matrics();
}

std::shared_ptr<DRL::Texture2D>
PBOBench::BenchUpload(TextureUploadingMode mode) {
  static double latency = 0;
  static uint64_t FrameIdx = 0;
  // 1024 x 1024
  static auto originalTexture = resMgr.find_path("spot_texture.png");
  static auto DarkerTexture = resMgr.find_path("spot_texture_dark.png");
  auto tp = std::chrono::high_resolution_clock::now();

  std::shared_ptr<DRL::Texture2D> Res;
  static int width, height, nrChannels;
  static unsigned char *data0 = stbi_load(originalTexture.u8string().c_str(),
                                          &width, &height, &nrChannels, 0);
  assert(nrChannels == 3);
  static unsigned char *data1 = stbi_load(DarkerTexture.u8string().c_str(),
                                          &width, &height, &nrChannels, 0);
  assert(nrChannels == 3);

  auto DataSize = width * height * sizeof(unsigned char) * nrChannels;
  if (PBOBuffers[0] == 0) {
    glCreateBuffers(2, PBOBuffers);
    glNamedBufferStorage(PBOBuffers[0], DataSize, nullptr,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glNamedBufferStorage(PBOBuffers[1], DataSize, nullptr,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  }
  switch (mode) {
  case TextureUploadingMode::Baseline: {
    static auto spot0 =
        std::make_shared<DRL::Texture2D>(originalTexture, 1, false, false);
    static auto spot1 =
        std::make_shared<DRL::Texture2D>(DarkerTexture, 1, false, false);
    Res = (FrameIdx++ % 2 == 0) ? spot0 : spot1;
    break;
  }
  case TextureUploadingMode::Sync: {
    // 在构造函数里会同步上传
    Res = std::make_shared<DRL::Texture2D>(
        (FrameIdx++ % 2) ? originalTexture : DarkerTexture, 1, false, false);
    break;
  }
  case TextureUploadingMode::PBO: {
    Res = std::make_shared<DRL::Texture2D>(1024, 1024, 1, GL_RGB8);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOBuffers[0]);
    // glBufferSubData(GL_PIXEL_UNPACK_BUFFER,0, DataSize, data0);
    GLubyte *ptr =
        (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (ptr) {
      memcpy(ptr, (FrameIdx++ % 2) ? data0 : data1, DataSize);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
    }

    glBindTexture(GL_TEXTURE_2D, Res->handle());
    // glMemoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024, GL_RGB,
                    GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    break;
  }
  case TextureUploadingMode::PBO_Interval: {
    static int PBOIndex = 0;
    Res = std::make_shared<DRL::Texture2D>(1024, 1024, 1, GL_RGB8);
    glBindTexture(GL_TEXTURE_2D, Res->handle());
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOBuffers[PBOIndex]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024, GL_RGB,
                    GL_UNSIGNED_BYTE, 0);

    PBOIndex = (PBOIndex + 1) % 2;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOBuffers[PBOIndex]);
    // glBufferData(GL_PIXEL_UNPACK_BUFFER,DataSize, 0, GL_STREAM_DRAW);
    GLubyte *ptr =
        (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (ptr) {
      memcpy(ptr, (FrameIdx++ % 2) ? data0 : data1, DataSize);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    break;
  }
  }

  auto tp1 = std::chrono::high_resolution_clock::now();
  // expotional moving averrage
  latency =
      0.9 * latency +
      0.1 * std::chrono::duration_cast<std::chrono::milliseconds>(tp1 - tp)
                .count();
  if (FrameIdx > 0 && FrameIdx % 300 == 0) {
    spdlog::info("Time for Uploding Texture: {}ms", latency);
  }
  return Res;
}

void PBOBench::render() {

  static bool BenchUploading = false;
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::InputInt("Draw Numbers", &DrawNumbers);

    ImGui::Checkbox("Bench Uploading", &BenchUploading);
    if (BenchUploading) {
      const char *modes[] = {"Baseline", "Sync", "PBO 1", "PBO 2"};
      ImGui::Combo("UpLoading Mode", (int *)&mUploadingMode, modes,
                   IM_ARRAYSIZE(modes));
    }
    ImGui::End();
  }
  ImGui::Render();
  if (oldDrawNumbers != DrawNumbers) {
    oldDrawNumbers = DrawNumbers;
    update_model_matrics();
  }
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  spotTexture = BenchUpload(mUploadingMode);
  spotTexture->bind();
  shader.bind();
  shader.set_uniform("texture_diffuse1", 0);
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 1000.0f);
  glm::mat4 view = camera_->GetViewMatrix();
  shader.set_uniform("projection", projection);
  shader.set_uniform("view", view);
  auto model_mat4 = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(),
                                glm::vec3(0.0f, 0.1f, 0.0f));
  for (int i = 0; i < DrawNumbers; i++) {
    shader.set_uniform("model", model_mat4 * modelMatrics[i]);
    spot_ptr->Draw(shader);
  }
}
void PBOBench::update_model_matrics() {

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
  if (modelMatricsVBO != 0) {
    glDeleteBuffers(1, &modelMatricsVBO);
  }
  glCreateBuffers(1, &modelMatricsVBO);
  glNamedBufferStorage(modelMatricsVBO, DrawNumbers * sizeof(glm::mat4),
                       modelMatrics.data(), GL_DYNAMIC_STORAGE_BIT);
  glNamedBufferSubData(modelMatricsVBO, 0, DrawNumbers * sizeof(glm::mat4),
                       modelMatrics.data());
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
  PBOBench rd(info);
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 50.0, 305.0});
  rd.loop();

  return 0;
}

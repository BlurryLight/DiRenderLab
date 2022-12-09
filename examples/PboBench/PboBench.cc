#include "PboBench.hh"
using namespace DRL;
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
#include <stb_image_write.h>

// 有两种做法:
// 1. Orphan
//  如文章内描述的http://www.songho.ca/opengl/gl_pbo.html#pack,
// Buffer创建为glBufferData创建，每次调用的时候先调用 glbufferData(...,nullptr,..)创建个新得Buffer内存
// 旧的内存如果其他地方仍在引用，则其他地方仍然维持这个引用，新的buffer可以直接写入
// 这样不会引入复杂的同步问题

// 2. Sync
// 通过Sync同步
GLuint PBOUploadingBuffers[2] = {0, 0};
GLsync PBOUploadingFence[2] = {nullptr,nullptr};

GLuint PBODownloadingBuffers[2] = {0, 0};
GLsync PBODownloadingFence[2] = {nullptr,nullptr};

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

static void HandleGLFence(GLsync& fence)
{
  if(glIsSync(fence))
  {
   GLenum result = glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
   {
    switch(result)
    {
      case GL_CONDITION_SATISFIED:
        spdlog::warn("wait for fence");
        break;
      // case GL_ALREADY_SIGNALED:
      case GL_TIMEOUT_EXPIRED:
      case GL_WAIT_FAILED:
        spdlog::error("something goes wrong");
        std::abort();
    }
   }
   glDeleteSync(fence);
   fence = nullptr;
  }
}



void PBOBench::BenchDownload (TextureTransferMode mode) {
  static double latency = 0;
  static uint64_t FrameIdx = 0;
  auto tp = std::chrono::high_resolution_clock::now();
  auto DataSize = info_.width * info_.height * sizeof(uint8_t) * 4;
  static void* data_ptr = malloc(DataSize);
  assert(data_ptr);
  if (PBODownloadingBuffers[0] == 0) {
    glCreateBuffers(2, PBODownloadingBuffers);
    glNamedBufferStorage(PBODownloadingBuffers[0], DataSize, nullptr,
                         GL_MAP_READ_BIT);
    glNamedBufferStorage(PBODownloadingBuffers[1], DataSize, nullptr,
                         GL_MAP_READ_BIT);
  }
  switch (mode)
  {
    case TextureTransferMode::Baseline:
    {
      // do nothing
      return;
    }
    case TextureTransferMode::Sync: {
      glReadnPixels(0,0,info_.width,info_.height,GL_RGBA,GL_UNSIGNED_BYTE,DataSize,data_ptr);
      break;
    }
    case TextureTransferMode::PBO: 
    {
      HandleGLFence(PBODownloadingFence[0]);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, PBODownloadingBuffers[0]);
      // 第一帧会读到空数据，问题不大
      GLubyte *ptr =
        (GLubyte *)glMapBufferRange(GL_PIXEL_PACK_BUFFER,0,DataSize,GL_MAP_READ_BIT);
        if (ptr) {
          memcpy(data_ptr, ptr, DataSize);
          glUnmapBuffer(GL_PIXEL_PACK_BUFFER); 
      }
      glReadnPixels(0,0,info_.width,info_.height,GL_RGBA,GL_UNSIGNED_BYTE,DataSize,0);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
      PBODownloadingFence[0] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      break;
    }
    case TextureTransferMode::PBO_Interval:
     {
        static int PBOIndex = 0;
        HandleGLFence(PBODownloadingFence[PBOIndex]);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, PBODownloadingBuffers[PBOIndex]);
        GLubyte *ptr =
          (GLubyte *)glMapBufferRange(GL_PIXEL_PACK_BUFFER,0,DataSize,GL_MAP_READ_BIT);
          if (ptr) {
            memcpy(data_ptr, ptr, DataSize);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER); 
        }

        PBOIndex = (PBOIndex + 1) % 2;
        auto NextIndex = PBOIndex;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, PBODownloadingBuffers[NextIndex]);
        glReadnPixels(0,0,info_.width,info_.height,GL_RGBA,GL_UNSIGNED_BYTE,DataSize,0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        PBODownloadingFence[NextIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        break;
    }
  }

  auto tp1 = std::chrono::high_resolution_clock::now();
  latency =
      0.9 * latency +
      0.1 * std::chrono::duration_cast<std::chrono::milliseconds>(tp1 - tp)
                .count();

  if(FrameIdx == 10)
  {
    stbi_flip_vertically_on_write(true);
    stbi_write_png("grab_col_buf.png", info_.width, info_.height, 4, data_ptr,4 * info_.width);
  }

  FrameIdx++;
  if (FrameIdx > 0 && FrameIdx % 300 == 0) {
    spdlog::info("Time for Downloading Texture: {}ms", latency);
  }

}

std::shared_ptr<DRL::Texture2D>
PBOBench::BenchUpload(TextureTransferMode mode) {
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
  if (PBOUploadingBuffers[0] == 0) {
    glCreateBuffers(2, PBOUploadingBuffers);
    glNamedBufferStorage(PBOUploadingBuffers[0], DataSize, nullptr,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    glNamedBufferStorage(PBOUploadingBuffers[1], DataSize, nullptr,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);

    PBOUploadingFence[0] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    PBOUploadingFence[1] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }
  switch (mode) {
  case TextureTransferMode::Baseline: {
    static auto spot0 =
        std::make_shared<DRL::Texture2D>(originalTexture, 1, false, false);
    static auto spot1 =
        std::make_shared<DRL::Texture2D>(DarkerTexture, 1, false, false);
    Res = (FrameIdx++ % 2 == 0) ? spot0 : spot1;
    break;
  }
  case TextureTransferMode::Sync: {
    Res = std::make_shared<DRL::Texture2D>(1024, 1024, 1, GL_RGB8);
    glBindTexture(GL_TEXTURE_2D, Res->handle());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024, GL_RGB,
                    GL_UNSIGNED_BYTE, (FrameIdx++ % 2) ? data0 : data1);
    glBindTexture(GL_TEXTURE_2D, 0);
    break;
  }
  case TextureTransferMode::PBO: {
    // 第一帧渲染的时候可能会出现纹理还没上传完的情况
    Res = std::make_shared<DRL::Texture2D>(1024, 1024, 1, GL_RGB8);
    //创建屏障
    HandleGLFence(PBOUploadingFence[0]);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOUploadingBuffers[0]);
    // glBufferSubData(GL_PIXEL_UNPACK_BUFFER,0, DataSize, data0);
    GLubyte *ptr =
        (GLubyte *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,DataSize,GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    if (ptr) {
      memcpy(ptr, (FrameIdx++ % 2) ? data0 : data1, DataSize);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
    }
    glBindTexture(GL_TEXTURE_2D, Res->handle());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024, GL_RGB,
                    GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    PBOUploadingFence[0] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    break;
  }
  case TextureTransferMode::PBO_Interval: {
    static int PBOIndex = 0;
    Res = std::make_shared<DRL::Texture2D>(1024, 1024, 1, GL_RGB8);
    glBindTexture(GL_TEXTURE_2D, Res->handle());
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOUploadingBuffers[PBOIndex]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024, GL_RGB,
                    GL_UNSIGNED_BYTE, 0);

    PBOIndex = (PBOIndex + 1) % 2;
    auto NextIndex = PBOIndex;

    // 驱动会sync 上传的问题，真正需要保护的是下面的 MapBufferRange。在Map之前我们必须保护所有的用到这个Buffer的命令已经执行完了，否则会看到Corrupted的数据 
    HandleGLFence(PBOUploadingFence[NextIndex]);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PBOUploadingBuffers[NextIndex]);
    // tragedy 2: orphaing
    // glBufferData(GL_PIXEL_UNPACK_BUFFER,DataSize, 0, GL_STREAM_DRAW);

    // tragedy 3: mannually Fence
    // GLubyte *ptr =
    //     (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    GLubyte *ptr =
        (GLubyte *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,DataSize,GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    if (ptr) {
      memcpy(ptr, (FrameIdx++ % 2) ? data0 : data1, DataSize);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    PBOUploadingFence[NextIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    break;
  }
  }

  if(FrameIdx > 1)
  {
    ImGui::Begin("Uploading Texture", 0);
      ImGui::Image((void*)(intptr_t)Res->handle(), ImVec2(256.0,256.0));
    ImGui::End();
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
  static bool BenchDownloading = false;
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::InputInt("Draw Numbers", &DrawNumbers);

    ImGui::Checkbox("Bench Uploading", &BenchUploading);
    if (BenchUploading) {
      BenchDownloading = false;
      static const char *modes[] = {"Baseline", "Sync", "PBO 1", "PBO 2"};
      ImGui::Combo("UpLoading Mode", (int *)&mUploadingMode, modes,
                   IM_ARRAYSIZE(modes));
    }
    ImGui::Checkbox("Bench Downloading", &BenchDownloading);
    if (BenchDownloading) {
      BenchUploading = false;
      static const char *modes[] = {"Baseline", "Sync", "PBO 1", "PBO 2"};
      ImGui::Combo("Downloading Mode", (int *)&mDownloadingMode, modes,
                   IM_ARRAYSIZE(modes));
    }
    ImGui::End();
  }

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
  if(BenchDownloading)
  {
    BenchDownload(mDownloadingMode);
  }
  ImGui::Render();
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

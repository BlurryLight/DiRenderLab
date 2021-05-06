//
// Created by zhong on 2021/5/3.
//

#include "pbrRender.hh"
#include "GLwrapper/shapes.hh"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
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
    ImGui::SliderFloat3("LightColor", glm::value_ptr(uniform_.lightColor), 0.0f,
                        500.0f, "%.3f");
    ImGui::SliderFloat3("LightPos", glm::value_ptr(uniform_.lightPos), 0.0f,
                        50.0f, "%.3f");
    ImGui::SliderFloat("metallic", &uniform_.metallic_index, 0.0f, 1.0f,
                       "%.3f");
    ImGui::SliderFloat("roughness", &uniform_.roughness_index, 0.0f, 1.0f,
                       "%.3f");
    ImGui::SliderFloat("lod level", &uniform_.skybox_lod, 0.0f, 4.0f, "%.3f");
    ImGui::End();
  }
  ImGui::Render();
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 view = camera_->GetViewMatrix();
  {
    DRL::bind_guard gd(pbrShader);

    uniform_.irradianceCubemap->set_slot(4);
    uniform_.irradianceCubemap->bind();
    pbrShader.set_uniform("projection", uniform_.proj);
    pbrShader.set_uniform("view", view);
    pbrShader.set_uniform("model", glm::mat4(1.0));
    pbrShader.set_uniform("camPos", camera_->Position);
    pbrShader.set_uniform("u_metallicMap",
                          uniform_.metallicARB.tex_handle_ARB());
    pbrShader.set_uniform("u_roughnessMap",
                          uniform_.roughnessARB.tex_handle_ARB());
    pbrShader.set_uniform("u_normalMap", uniform_.normalARB.tex_handle_ARB());
    pbrShader.set_uniform("u_albedoMap", uniform_.albedoARB.tex_handle_ARB());

    pbrShader.set_uniform("lightPosition", uniform_.lightPos);
    pbrShader.set_uniform("lightColor", uniform_.lightColor);

    pbrShader.set_uniform("u_metallic_index", uniform_.metallic_index);
    pbrShader.set_uniform("u_roughness_index", uniform_.roughness_index);
    model_ptr->Draw(pbrShader);
  }

  {
    DRL::bind_guard gd(lightShader);
    lightShader.set_uniform("projection", uniform_.proj);
    lightShader.set_uniform("view", view);
    auto model = glm::translate(glm::mat4(1.0), uniform_.lightPos);
    lightShader.set_uniform("model", glm::scale(model, glm::vec3(0.1)));
    DRL::renderSphere();
  }

  {
    skyboxShader.bind();
    skyboxShader.set_uniform("view", glm::mat4(glm::mat3(view)));
    skyboxShader.set_uniform("projection", uniform_.proj);
    //    uniform_.prefilterCubemap->bind();
    skyboxShader.set_uniform("u_lod_level", uniform_.skybox_lod);
    //    uniform_.prefilterCubemap->bind();
    uniform_.envCubemap->bind();
    //    uniform_.irradianceCubemap->set_slot(0);
    //    uniform_.irradianceCubemap->bind();
    renderCube();
  }
}
void PbrRender::setup_states() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures" /
                  "pbr");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures" /
                  "pbr" / "envmap" / "factory");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "pbrRender");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "skybox");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "models" /
                  "basic");
  pbrShader = DRL::make_program(resMgr.find_path("pbr.vert"),
                                resMgr.find_path("pbr_IBL.frag"));

  lightShader = DRL::make_program(resMgr.find_path("pbr.vert"),
                                  resMgr.find_path("light.frag"));
  skyboxShader = DRL::make_program(resMgr.find_path("skybox.vert"),
                                   resMgr.find_path("skybox.frag"));

  equirectangularToCubemapShader = DRL::make_program(
      resMgr.find_path("cube.vert"), resMgr.find_path("sphereTo2D.frag"));

  irradianceConvShader = DRL::make_program(
      resMgr.find_path("cube.vert"), resMgr.find_path("calc_convolution.frag"));

  prefilterShader = DRL::make_program(resMgr.find_path("cube.vert"),
                                      resMgr.find_path("prefilter.frag"));
  uniform_.proj =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 100.0f);
  uniform_.lightPos = glm::vec3(10.0f);
  uniform_.lightColor = glm::vec3(200.0f);

  model_ptr =
      std::make_unique<DRL::Model>(resMgr.find_path("sphere.obj").string());
  uniform_.albedoARB = DRL::Texture2DARB(
      resMgr.find_path("rustediron2_basecolor.png"), false, false);
  uniform_.albedoARB.make_resident();
  uniform_.roughnessARB = DRL::Texture2DARB(
      resMgr.find_path("rustediron2_roughness.png"), false, false);
  uniform_.roughnessARB.make_resident();
  uniform_.normalARB = DRL::Texture2DARB(
      resMgr.find_path("rustediron2_normal.png"), false, false);
  uniform_.normalARB.make_resident();
  uniform_.metallicARB = DRL::Texture2DARB(
      resMgr.find_path("rustediron2_metallic.png"), false, false);
  uniform_.metallicARB.make_resident();

  uniform_.hdrTexture = DRL::Texture2D(
      resMgr.find_path("Factory_Catwalk_2k.hdr").string(), true, false);
  auto envCubemap = std::make_shared<DRL::TextureCube>(
      2048, 2048, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
  uniform_.envCubemap = envCubemap;
  uniform_.envCubemap->generateMipmap();

  glm::mat4 captureProjection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  glm::mat4 captureViews[] = {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f))};

  // convert HDR equirectangular environment map to cubemap equivalent
  equirectangularToCubemapShader.bind();
  equirectangularToCubemapShader.set_uniform("equirectangularMap", 0);
  equirectangularToCubemapShader.set_uniform("projection", captureProjection);
  uniform_.hdrTexture.bind();

  // pbr: setup framebuffer
  // ----------------------

  uniform_.captureFBO.set_viewport(envCubemap, 0);
  for (unsigned int i = 0; i < 6; ++i) {
    equirectangularToCubemapShader.set_uniform("view", captureViews[i]);
    uniform_.captureFBO.attach_buffer(GL_COLOR_ATTACHMENT0, envCubemap, 0, i);
    uniform_.captureFBO.bind();
    renderCube(); // renders a 1x1 cube
  }
  uniform_.captureFBO.unbind();
  // calc irradiance

  uniform_.irradianceCubemap = std::make_shared<DRL::TextureCube>(
      32, 32, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
  uniform_.irradianceCubemap->set_wrap_t(GL_CLAMP_TO_EDGE);
  uniform_.irradianceCubemap->set_wrap_s(GL_CLAMP_TO_EDGE);
  uniform_.irradianceCubemap->set_wrap_r(GL_CLAMP_TO_EDGE);
  irradianceConvShader.bind();
  irradianceConvShader.set_uniform("environmentMap", 0);
  irradianceConvShader.set_uniform("projection", captureProjection);
  envCubemap->bind();
  uniform_.captureFBO.set_viewport(32, 32);
  for (unsigned int i = 0; i < 6; ++i) {
    irradianceConvShader.set_uniform("view", captureViews[i]);
    uniform_.captureFBO.attach_buffer(GL_COLOR_ATTACHMENT0,
                                      uniform_.irradianceCubemap, 0, i);
    uniform_.captureFBO.bind();
    renderCube(); // renders a 1x1 cube
  }

  // calc prefilter
  uniform_.prefilterCubemap = std::make_shared<DRL::TextureCube>(
      128, 128, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
  uniform_.prefilterCubemap->generateMipmap();
  uniform_.prefilterCubemap->set_min_filter(GL_LINEAR_MIPMAP_LINEAR);
  // filter to mip_linear
  prefilterShader.bind();
  prefilterShader.set_uniform("environmentMap", 0);
  prefilterShader.set_uniform("projection", captureProjection);
  envCubemap->bind();
  envCubemap->generateMipmap();
  int prefilter_size = 128;
  int miplevels = 5;
  for (int mip = 0; mip < miplevels; mip++) {
    uniform_.captureFBO.set_viewport(prefilter_size * pow(0.5, mip),
                                     prefilter_size * pow(0.5, mip));
    float roughness = (float)mip / (float)miplevels;
    prefilterShader.set_uniform("roughness", roughness);
    for (unsigned int i = 0; i < 6; ++i) {
      prefilterShader.set_uniform("view", captureViews[i]);
      uniform_.captureFBO.attach_buffer(GL_COLOR_ATTACHMENT0,
                                        uniform_.prefilterCubemap, mip, i);
      uniform_.captureFBO.bind();
      renderCube(); // renders a 1x1 cube
    }
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  uniform_.captureFBO.unbind();
  glViewport(0, 0, info_.width, info_.height);
}

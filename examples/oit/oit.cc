//
// Created by zhong on 2021/5/10.
//

#include "oit.hh"
#include "GLwrapper/shapes.hh"
#include "third_party/imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>

#define QUAD_NUM 10
#define STEP 1
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
  rd.camera_ = std::make_unique<DRL::Camera>(glm::vec3{0.0, 0.0, 10.0});
  rd.loop();

  return 0;
}
void OitRender::setup_states() {
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "shaders" /
                  "oit");
  resMgr.add_path(decltype(resMgr)::root_path / "resources" / "textures" /
                  "oit");

  transparent_texture_ = std::make_shared<DRL::Texture2D>(
      resMgr.find_path("blending_transparent_window.png"), 1, false, false);
  solid_shader_ =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("mvp_pos_normal_texture.frag"));
  sorted_blend_shader_ =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("mvp_pos_n_t_sampler.frag"));
  transparent_shader_ =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("oit_transparent.frag"));

  composite_shader_ = DRL::make_program(resMgr.find_path("screen_quad.vert"),
                                        resMgr.find_path("oit_composite.frag"));
  screen_shader_ = DRL::make_program(resMgr.find_path("screen_quad.vert"),
                                     resMgr.find_path("screen_quad.frag"));
  dp_final_shader_ = DRL::make_program(resMgr.find_path("screen_quad.vert"),
                                       resMgr.find_path("dp_finalshader.frag"));

  dp_blend_shader_ = DRL::make_program(resMgr.find_path("screen_quad.vert"),
                                       resMgr.find_path("dp_blend.frag"));
  dp_peeling_shader_ =
      DRL::make_program(resMgr.find_path("mvp_pos_normal_texture.vert"),
                        resMgr.find_path("dp_front_peel.frag"));
  opaque_texture_ = std::make_shared<DRL::Texture2D>(info_.width, info_.height,
                                                     1, GL_RGBA32F);
  accum_texture_ = std::make_shared<DRL::Texture2D>(info_.width, info_.height,
                                                    1, GL_RGBA32F);
  reveal_texture_ =
      std::make_shared<DRL::Texture2D>(info_.width, info_.height, 1, GL_R32F);
  depth_texture_ = std::make_shared<DRL::Texture2D>(info_.width, info_.height,
                                                    1, GL_DEPTH_COMPONENT32F);
  transparent_texture_->set_slot(0);
  accum_texture_->set_slot(0);
  reveal_texture_->set_slot(1);
  opaque_texture_->set_slot(0);

  opaque_fbo_.attach_buffer(GL_COLOR_ATTACHMENT0, opaque_texture_, 0);
  opaque_fbo_.attach_buffer(GL_DEPTH_ATTACHMENT, depth_texture_, 0);
  opaque_fbo_.set_viewport(info_.width, info_.height);
  opaque_fbo_.clear_color_ = glm::vec4(0.1, 0.1, 0.1, 1.0);
  opaque_fbo_.clear_when_bind = false;

  transparent_fbo_.attach_buffer(GL_COLOR_ATTACHMENT0, accum_texture_, 0);
  transparent_fbo_.attach_buffer(GL_COLOR_ATTACHMENT1, reveal_texture_, 0);
  transparent_fbo_.attach_buffer(GL_DEPTH_ATTACHMENT, depth_texture_, 0);
  transparent_fbo_.set_viewport(info_.width, info_.height);
  transparent_fbo_.set_draw_buffer(
      {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
  transparent_fbo_.clear_color_ = glm::vec4(0.1, 0.1, 0.1, 1.0);
  transparent_fbo_.clear_when_bind = false;

  // init depth peeling
  for (int i = 0; i < 2; i++) {
    dp_color_texes_[i] = std::make_shared<DRL::Texture2D>(
        info_.width, info_.height, 1, GL_RGBA32F);
    dp_depth_texes_[i] = std::make_shared<DRL::Texture2D>(
        info_.width, info_.height, 1, GL_DEPTH_COMPONENT32);
    dp_fbos_[i].set_viewport(info_.width, info_.height);
    dp_fbos_[i].attach_buffer(GL_COLOR_ATTACHMENT0, dp_color_texes_[i], 0);
    dp_fbos_[i].attach_buffer(GL_DEPTH_ATTACHMENT, dp_depth_texes_[i], 0);
    dp_fbos_[i].clear_when_bind = false;
  }
  dp_color_blender_tex = std::make_shared<DRL::Texture2D>(
      info_.width, info_.height, 1, GL_RGBA32F);
  dp_color_blender_fbo_.attach_buffer(GL_COLOR_ATTACHMENT0,
                                      dp_color_blender_tex, 0);
  dp_color_blender_fbo_.attach_buffer(GL_DEPTH_ATTACHMENT, dp_depth_texes_[0],
                                      0);
  dp_color_blender_fbo_.set_viewport(info_.width, info_.height);
  dp_color_blender_fbo_.clear_when_bind = false;
  for (int i = QUAD_NUM; i >= 0; i--) {
    quad_pos_.emplace_back(0, 0.0, STEP * i);
  }
  for (int i = 1; i >= 0; i--) {
    solid_quad_pos_.emplace_back(0.0, 0.0, -6 + 12 * i);
  }
}

void OitRender::render() {

  static int method_index = 2;
  ImGui::NewFrame();
  {
    ImGui::Begin("Background Color", 0); // Create a window called "Hello,
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    //    ImGui::Checkbox("Weighted Blended OIT", &oit_);
    {
      const char *itemName[] = {"sorted Render", "weighted blended oit",
                                "depth peeling"};
      ImGui::Combo("Render Method", &method_index, itemName,
                   IM_ARRAYSIZE(itemName));
    }
    ImGui::End();
  }
  ImGui::Render();

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  std::vector<glm::mat4> solid_model_mat4s;
  for (const auto &solid_pos : solid_quad_pos_) {
    auto model = glm::translate(glm::mat4(1.0f), solid_pos);
    model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(1, 0, 0));
    solid_model_mat4s.push_back(model);
  }
  auto view = camera_->GetViewMatrix();
  auto proj =
      glm::perspective(glm::radians(camera_->Zoom),
                       (float)info_.width / (float)info_.height, 0.1f, 500.0f);
  //  if (!oit_) {
  //    back_to_front_render(solid_model_mat4s, view, proj);
  //
  //  } else {
  //  }
  switch (method_index) {
  case 0:
    back_to_front_render(solid_model_mat4s, view, proj);
    break;
  case 1:
    weighted_blended_render(solid_model_mat4s, view, proj);
    break;
  case 2:
    depth_peeling_render(solid_model_mat4s, view, proj);
    break;
  default:
    // other doesn't implement
    spdlog::error("Not Implemented! Fallback to weighted_blended_render");
    weighted_blended_render(solid_model_mat4s, view, proj);
    break;
  }
}
void OitRender::back_to_front_render(
    const std::vector<glm::mat4> &solid_model_mat4s, const glm::mat4 &view,
    const glm::mat4 &proj) {
  glEnable(GL_DEPTH_TEST);
  //????????????????????????
  {
    DRL::bind_guard gd(solid_shader_);
    solid_shader_.set_uniform("view", view);
    solid_shader_.set_uniform("projection", proj);
    for (auto &model : solid_model_mat4s) {
      solid_shader_.set_uniform("model", model);
      DRL::renderQuad();
    }
  }

  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // sorted_blend_shader_???frag
    // FragColor = texture(texture0, fs_in.TexCoords)
    //????????????alpha???????????????????????????
    DRL::bind_guard gd(sorted_blend_shader_, transparent_texture_);
    sorted_blend_shader_.set_uniform("view", view);
    sorted_blend_shader_.set_uniform("projection", proj);
    std::map<float, glm::vec3> sorted_quad;
    //????????????????????????????????????????????????????????????
    for (const auto &pos : quad_pos_) {
      float distance = glm::distance(pos, camera_->Position);
      sorted_quad[distance] = pos;
    }
    //???????????????????????????????????????????????????????????????
    glDepthMask(false);
    //?????????????????????????????????????????????
    for (auto it = sorted_quad.rbegin(); it != sorted_quad.rend(); it++) {
      auto i_model = glm::translate(glm::mat4(1.0f), it->second);
      i_model =
          glm::rotate(i_model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
      sorted_blend_shader_.set_uniform("model", i_model);
      DRL::renderQuad();
    }
    glDepthMask(true);
  }
}
void OitRender::weighted_blended_render(
    const std::vector<glm::mat4> &solid_model_mat4s, const glm::mat4 &view,
    const glm::mat4 &proj) {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  // ???????????????????????????
  {
    DRL::bind_guard gd(opaque_fbo_, solid_shader_);
    // we must mannully clear opaque_fbo_
    opaque_fbo_.clear();
    solid_shader_.set_uniform("view", view);
    solid_shader_.set_uniform("projection", proj);
    for (auto &solid_model_mat4 : solid_model_mat4s) {
      solid_shader_.set_uniform("model", solid_model_mat4);
      DRL::renderQuad();
    }
  }

  glDepthMask(GL_FALSE);
  // we need the depth test,but ban the depth writing.
  // when transparent obj is behind the solid objs, they should be discarded.
  glEnable(GL_BLEND);
  // glBlendFunci????????????MRT?????????Buffer?????????????????????4.0??????
  glBlendFunci(0, GL_ONE, GL_ONE);
  glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
  glBlendEquation(GL_FUNC_ADD);
  {
    DRL::bind_guard gd(transparent_fbo_, transparent_shader_,
                       transparent_texture_);
    glClearNamedFramebufferfv(transparent_fbo_, GL_COLOR, 0,
                              glm::value_ptr(glm::vec4(0.0f)));
    glClearNamedFramebufferfv(transparent_fbo_, GL_COLOR, 1,
                              glm::value_ptr(glm::vec4(1.0f)));
    glClearNamedFramebufferfv(transparent_fbo_, GL_DEPTH, 0,
                              &transparent_fbo_.clear_depth_);
    transparent_shader_.set_uniform("view", view);
    transparent_shader_.set_uniform("projection", proj);

    for (const auto &pos : quad_pos_) {
      auto i_model = glm::translate(glm::mat4(1.0f), pos);
      i_model =
          glm::rotate(i_model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
      transparent_shader_.set_uniform("model", i_model);
      DRL::renderQuad();
    }
  }
  {
    DRL::bind_guard gd(opaque_fbo_, composite_shader_, accum_texture_,
                       reveal_texture_);
    // opaque_fbo_?????????solid?????????????????????alpha???1
    glEnable(GL_BLEND);
    // SRC???fragment??????????????????Dst???FBO??????????????????
    // ?????????????????????transparent * (1 - ??(1 - a))  + solid * (??(1 - a))
    // ??????????????????????????????????????????transparent?????????
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DRL::renderScreenQuad();
  }
  {
    //?????????????????????
    DRL::bind_guard gd(screen_shader_, opaque_texture_);
    DRL::renderScreenQuad();
  }
}
void OitRender::depth_peeling_render(
    const std::vector<glm::mat4> &solid_model_mat4s, const glm::mat4 &view,
    const glm::mat4 &proj) {
  auto draw_scene = [&](Program &shader) {
    //    solid_shader_.bind();
    //    solid_shader_.set_uniform("view", view);
    //    solid_shader_.set_uniform("projection", proj);
    //    for (auto &solid_model_mat4 : solid_model_mat4s) {
    //      solid_shader_.set_uniform("model", solid_model_mat4);
    //      DRL::renderQuad();
    //    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DRL::bind_guard gd(shader, transparent_texture_);
    shader.set_uniform("view", view);
    shader.set_uniform("projection", proj);
    for (const auto &pos : quad_pos_) {
      auto i_model = glm::translate(glm::mat4(1.0f), pos);
      i_model =
          glm::rotate(i_model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
      shader.set_uniform("model", i_model);
      DRL::renderQuad();
    }
  };

  dp_color_blender_fbo_.bind();
  dp_color_blender_fbo_.clear();
  // get the first layer
  glEnable(GL_DEPTH_TEST);
  draw_scene(sorted_blend_shader_);
  dp_color_blender_fbo_.unbind();

#define NUM_PASSES 3
  int numLayers = (NUM_PASSES - 1) * 2;
  for (int layer = 1; layer < numLayers; layer++) {
    int currId = layer % 2;
    int prevId = 1 - currId;
    dp_fbos_[currId].bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    dp_depth_texes_[prevId]->set_slot(1);
    //    dp_color_texes_[prevId]->set_slot(0);
    dp_depth_texes_[prevId]->bind();
    //    dp_color_texes_[prevId]->bind();
    draw_scene(dp_peeling_shader_);
    //    dp_color_texes_[prevId]->set_slot(0);
    dp_depth_texes_[prevId]->set_slot(0);
    dp_depth_texes_[prevId]->unbind();
    //    dp_color_texes_[prevId]->unbind();

    dp_color_blender_fbo_.bind();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // use separate blending function
    glBlendEquation(GL_FUNC_ADD);

    glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
    dp_color_texes_[currId]->set_slot(0);
    dp_color_texes_[currId]->bind();
    dp_blend_shader_.bind();
    DRL::renderScreenQuad();
    glDisable(GL_BLEND);
  }
  //  // final pass
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK_LEFT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  dp_color_blender_tex->set_slot(0);
  DRL::bind_guard gd(dp_final_shader_, dp_color_blender_tex);
  //  dp_final_shader_.set_uniform("vBackgroundColor", glm::vec4(0.0));
  DRL::renderScreenQuad();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
}

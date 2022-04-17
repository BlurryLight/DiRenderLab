//
// Created by zhong on 2021/5/10.
//

#ifndef DIRENDERLAB_OIT_HH
#define DIRENDERLAB_OIT_HH

#include "GLwrapper/framebuffer.hh"
#include "GLwrapper/glsupport.hh"
class OitRender : public DRL::RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program blend_shader_;
  DRL::Program solid_shader_;
  DRL::Program transparent_shader_;
  DRL::Program composite_shader_;
  DRL::Program screen_shader_;
  std::shared_ptr<DRL::Texture2D> transparent_texture_;
  std::shared_ptr<DRL::Texture2D> opaque_texture_;
  std::shared_ptr<DRL::Texture2D> accum_texture_;
  std::shared_ptr<DRL::Texture2D> reveal_texture_;
  std::shared_ptr<DRL::Texture2D> depth_texture_;
  DRL::Framebuffer opaque_fbo_;
  DRL::Framebuffer transparent_fbo_;
  std::vector<glm::vec3> quad_pos_;
  std::vector<glm::vec3> solid_quad_pos_;
  //  bool oit_ = true;

public:
  OitRender() = default;
  explicit OitRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;

private:
  void back_to_front_render(const std::vector<glm::mat4> &solid_model_mat4s,
                            const glm::mat4 &view, const glm::mat4 &proj);
  void weighted_blended_render(const std::vector<glm::mat4> &solid_model_mat4s,
                               const glm::mat4 &view, const glm::mat4 &proj);
};

#endif // DIRENDERLAB_OIT_HH

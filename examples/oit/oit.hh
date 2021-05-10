//
// Created by zhong on 2021/5/10.
//

#ifndef DIRENDERLAB_OIT_HH
#define DIRENDERLAB_OIT_HH

#include "GLwrapper/glsupport.hh"
class OitRender : public DRL::RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program blend_shader_;
  DRL::Program pos_shader_;
  std::shared_ptr<DRL::Texture2DARB> transparent_texture_;
  std::vector<glm::vec3> quad_pos_;

public:
  OitRender() = default;
  explicit OitRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};

#endif // DIRENDERLAB_OIT_HH

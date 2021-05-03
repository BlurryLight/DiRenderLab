//
// Created by zhong on 2021/5/3.
//

#ifndef DIRENDERLAB_PBRRENDER_HH
#define DIRENDERLAB_PBRRENDER_HH

#include "GLwrapper/global.hh"
#include "GLwrapper/glsupport.hh"
#include "GLwrapper/shapes.hh"
#include "GLwrapper/texture.hh"

namespace DRL {

class PbrRender : public RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  float metallic_ = 0.0f;
  float roughness_ = 0.0f;
  //  DRL::VertexArray pbr_vao;

public:
  PbrRender() = default;
  explicit PbrRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};
} // namespace DRL

#endif // DIRENDERLAB_PBRRENDER_HH

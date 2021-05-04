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
struct uniform_block {
  float metallic_ = 0.0f;
  float roughness_ = 0.0f;
  glm::vec3 lightPos{};
  glm::vec3 lightColor{};
  //  std::vector<glm::vec3> lightPos;
  //  std::vector<glm::vec3> lightColors;
  glm::mat4 proj{1.0};
};
class PbrRender : public RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program pbrShader;
  DRL::Program lightShader; // for visualize light sphere
  //  DRL::VertexArray pbr_vao;
  uniform_block uniform_;

public:
  PbrRender() = default;
  explicit PbrRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};
} // namespace DRL

#endif // DIRENDERLAB_PBRRENDER_HH

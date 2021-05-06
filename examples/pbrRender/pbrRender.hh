//
// Created by zhong on 2021/5/3.
//

#ifndef DIRENDERLAB_PBRRENDER_HH
#define DIRENDERLAB_PBRRENDER_HH

#include "GLwrapper/global.hh"
#include "GLwrapper/glsupport.hh"
#include "GLwrapper/shapes.hh"
#include "GLwrapper/texture.hh"
#include <GLwrapper/framebuffer.hh>

namespace DRL {
struct uniform_block {
  float metallic_index = 1.0f;
  float roughness_index = 1.0f;
  glm::vec3 lightPos{};
  glm::vec3 lightColor{};
  //  std::vector<glm::vec3> lightPos;
  //  std::vector<glm::vec3> lightColors;
  glm::mat4 proj{1.0};
  DRL::Texture2DARB albedoARB;
  //  DRL::Texture2DARB aoARB;
  DRL::Texture2DARB normalARB;
  DRL::Texture2DARB metallicARB;
  DRL::Texture2DARB roughnessARB;

  DRL::Texture2D hdrTexture;
  DRL::Framebuffer captureFBO;
  std::shared_ptr<TextureCube> envCubemap{nullptr};
};
class PbrRender : public RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program pbrShader;
  DRL::Program lightShader; // for visualize light sphere
  DRL::Program equirectangularToCubemapShader;
  DRL::Program skyboxShader;
  //  DRL::VertexArray pbr_vao;
  uniform_block uniform_;
  std::unique_ptr<DRL::Model> model_ptr = nullptr;

public:
  PbrRender() = default;
  explicit PbrRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};
} // namespace DRL

#endif // DIRENDERLAB_PBRRENDER_HH
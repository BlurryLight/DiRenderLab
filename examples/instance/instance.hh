//
// Created by zhong on 2021/5/1.
//

#ifndef DIRENDERLAB_INSTANCE_HH
#define DIRENDERLAB_INSTANCE_HH
#include "GLwrapper/global.hh"
#include "GLwrapper/glsupport.hh"
#include "GLwrapper/texture.hh"
#include "GLwrapper/vertex_array.hh"
#include "Utils/text_overlay.hh"
using DRL::RenderBase;
class InstanceRender : public RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  DRL::Program InstanceShader;
  DRL::VertexArray asteroidsVAO;
  std::unique_ptr<DRL::Model> spot_ptr;
  int DrawNumbers = 1'00;
  int oldDrawNumbers = DrawNumbers;
  std::vector<glm::mat4> modelMatrics;
  //    DRL::VertexBuffer modelMatricsVBO;
  unsigned int modelMatricsVBO = 0;
  DRL::Texture2D spotTexture;

  enum Mode { kDraw = 0, kInstance = 1, kIndirectInstance = 2 };
  int mode_ = kInstance;
  void update_model_matrics();
  std::string mScreenText;
  DRL::TextOverlay mTextOverlay;

public:
  InstanceRender() = default;
  explicit InstanceRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};

#endif // DIRENDERLAB_INSTANCE_HH

//
// Created by zhong on 2021/4/30.
//

#ifndef DIRENDERLAB_SKYBOX_HH
#define DIRENDERLAB_SKYBOX_HH

#include "GLwrapper/global.hh"
#include "GLwrapper/glsupport.hh"
#include "GLwrapper/shapes.hh"
#include "GLwrapper/texture.hh"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_glfw.h"
#include "third_party/imgui/imgui_impl_opengl3.h"

using DRL::RenderBase;

class SkyboxRender : public RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  DRL::Program skyboxShader;
  DRL::Program skyboxShader2;
  int skyboxMode = 0;
  DRL::VertexArray skyboxVAO;
  DRL::TextureCube skyboxTexture;

public:
  SkyboxRender() = default;
  explicit SkyboxRender(const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};

#endif // DIRENDERLAB_SKYBOX_HH

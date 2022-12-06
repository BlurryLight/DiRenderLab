#pragma once

#include "GLwrapper/global.hh"
#include "GLwrapper/glsupport.hh"
#include "GLwrapper/texture.hh"
#include "GLwrapper/vertex_array.hh"
using DRL::RenderBase;
class PBOBench : public RenderBase {
protected:
  DRL::ResourcePathSearcher resMgr;
  DRL::Program shader;
  DRL::VertexArray asteroidsVAO;
  std::unique_ptr<DRL::Model> spot_ptr;
  int DrawNumbers = 1'00;
  int oldDrawNumbers = DrawNumbers;
  std::vector<glm::mat4> modelMatrics;
  //    DRL::VertexBuffer modelMatricsVBO;
  unsigned int modelMatricsVBO = 0;
  std::shared_ptr<DRL::Texture2D> spotTexture;

  enum class TextureUploadingMode : int 
  {
      Baseline = 0, // 纹理一开始就上传了
      Sync,
      PBO,
      PBO_Interval,
  };
  TextureUploadingMode mUploadingMode = TextureUploadingMode::Baseline;
  std::shared_ptr<DRL::Texture2D> BenchUpload(TextureUploadingMode mode);
  void update_model_matrics();

public:
  PBOBench () = default;
  explicit PBOBench (const BaseInfo &info) : DRL::RenderBase(info) {}
  void setup_states() override;
  void render() override;
};


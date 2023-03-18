//
// Created by zhong on 2021/4/23.
//

#ifndef DIRENDERLAB_SHADER_HH
#define DIRENDERLAB_SHADER_HH

#include "globject.hh"
#include "utils/resource_path_searcher.h" // for path searcher
#include <glm/glm.hpp>
#include <string>
namespace DRL {

enum ShaderType : int {
  kVShader = 0,
  kGShader = 1,
  kFShader = 2,
  kCShader = 3,
  kOtherShader = 4,
};
inline ShaderType MapGLEnumToShaderType(GLenum t) {
  switch (t) {
  case GL_VERTEX_SHADER:
    return kVShader;
  case GL_FRAGMENT_SHADER:
    return kFShader;
  case GL_GEOMETRY_SHADER:
    return kGShader;
  case GL_COMPUTE_SHADER:
    return kCShader;
  default:
    return kOtherShader;
  }
}
inline std::string MapShaderTypeToString(ShaderType t) {
  switch (t) {
  case kVShader:
    return {"VShader"};
  case kFShader:
    return {"FShader"};
  case kGShader:
    return {"GShader"};
  case kCShader:
    return {"ComputeShader"};
  case kOtherShader:
  default:
    return "Unknown shader";
  }
}

class Shader {
private:
  // for debug use
  std::string content_{};

protected:
  bool compiled_ = false;
  // a flag need to be checked before using
  ShaderObj obj_; // RAII handle
  ShaderType type_;

public:
  ~Shader() {
    // It maybe a bug that a shader was created but never compiled!
    // check it is compiled or be moved
    AssertLog(compiled_ || (obj_.handle() == 0),
              "Shader {} is never compiled!");
  }
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  Shader(Shader &&other) = default;
  Shader &operator=(Shader &&) = default;
  // the default behaviour is compile the shader in constructor
  Shader(GLenum type, const std::string &content, bool compile = true);
  Shader(GLenum type, const fs::path &path, bool compile = true);
  void compile();
  [[nodiscard]] bool compiled() const { return compiled_; }
  [[nodiscard]] ShaderType type() const { return type_; }
};

} // namespace DRL

#endif // DIRENDERLAB_SHADER_HH

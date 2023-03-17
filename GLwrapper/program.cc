//
// Created by zhong on 2021/4/24.
//

#include "program.hh"
#include "glm/gtc/type_ptr.hpp"
using namespace DRL;
#include <cstdlib>
// init static variable
Program *Program::current_using_program = nullptr;
void Program::link() {
  AssertLog(!linked_, "Program {} linked multiple times!", obj_.handle());
  if (!(shaders_bits_[0] && shaders_bits_[2])) // check vshader and fshader
  {
    spdlog::warn("Be cautious! Program {} lacks vshader or fshader!", obj_);
  }
  glLinkProgram(obj_);
  GLint status;
  glGetProgramiv(obj_, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLint maxLength = 0;
    glGetProgramiv(obj_, GL_INFO_LOG_LENGTH, &maxLength);
    // The maxLength includes the NULL character
    std::string infoLog(maxLength, '0');
    glGetProgramInfoLog(obj_, maxLength, nullptr, infoLog.data());
    spdlog::error("{} PROGRAM_LINK_ERROR: {}", obj_, infoLog);
    spdlog::shutdown();
    std::quick_exit(-1);
  }
  linked_ = true;
}
Program::Program(
    std::initializer_list<std::reference_wrapper<const Shader>> shaders) {
  this->attach_shaders(shaders);
}
void Program::attach_shaders(
    std::initializer_list<std::reference_wrapper<const Shader>> shaders) {
  // shader is a const reference
  AssertLog(!linked_, "AttachShader on an linked program!");
  for (auto shader : shaders) {
    if (!shader.get().compiled()) {
      spdlog::warn("Shader {} is not compiled when attaching!",
                   shader.get().handle());
    }
    auto check_bits = [this](ShaderType t) {
      AssertLog(!this->shaders_bits_[t],
                "{} Shader is attached by multiple times!",
                MapShaderTypeToString(t));
    };
    auto type = shader.get().type();
    auto shader_handle = shader.get().handle();
    if (type == kVShader || type == kFShader || type == kGShader) {
      check_bits(type);
      shaders_bits_[type] = true;
    }
    glAttachShader(obj_, shader_handle);
  }
}
// from cppreference
// magic here
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
void Program::set_uniform(std::string_view name,
                          const Program::Uniform_t &value) const {
  AssertLog(isBounded(),
            "Program {} is setting uniform, but  it has yet been bounded!",
            obj_);
  GLint loc = -1;
  std::string key(name);
  if(uniform_sheets_.find(key) != uniform_sheets_.end())
  {
    loc = uniform_sheets_.at(key);
  }
  else
  {
    loc = glGetUniformLocation(obj_, name.data());
    uniform_sheets_[key] = loc;
  }
  AssertLog(loc != -1,
            "Program {} set value {} failed because "
            "it does not correspond to an active uniform variable.",
            obj_, name);
  // code from https://github.com/rioki/glow/blob/master/glow/Shader.cpp
  std::visit(
      overloaded{
          [&](bool v) { glUniform1i(loc, v); },
          [&](int v) { glUniform1i(loc, v); },
          [&](glm::uint v) { glUniform1ui(loc, v); },
          [&](float v) { glUniform1f(loc, v); },
          //                       [&](const glm::ivec2 v) { glUniform2i(loc,
          //                       v.x, v.y); },
          //                       [&](const glm::uvec2 v) { glUniform2i(loc,
          //                       v.x, v.y); },
          [&](const glm::vec2 v) { glUniform2f(loc, v.x, v.y); },
          //                       [&](const glm::ivec3 v) { glUniform3i(loc,
          //                       v.x, v.y, v.z); },
          //                       [&](const glm::uvec3 v) { glUniform3i(loc,
          //                       v.x, v.y, v.z); },
          [&](const glm::vec3 v) { glUniform3f(loc, v.x, v.y, v.z); },
          [&](const glm::ivec4 v) { glUniform4i(loc,
                                v.x, v.y, v.z, v.w); },
          //                       [&](const glm::uvec4 v) { glUniform4i(loc,
          //                       v.x, v.y, v.z, v.w); },
          [&](const glm::vec4 v) { glUniform4f(loc, v.x, v.y, v.z, v.w); },
          [&](const glm::mat2 v) {
            glUniformMatrix2fv(loc, 1u, GL_FALSE, glm::value_ptr(v));
          },
          [&](const glm::mat3 v) {
            glUniformMatrix3fv(loc, 1u, GL_FALSE, glm::value_ptr(v));
          },
          [&](const glm::mat4 v) {
            glUniformMatrix4fv(loc, 1u, GL_FALSE, glm::value_ptr(v));
          },
#ifdef GL_ARB_BINDLESS
          [&](const GLuint64 v) { glUniformHandleui64ARB(loc, v); }
#endif
      },
      value);
}

Program DRL::make_program(const fs::path &vpath,
                          std::optional<const fs::path> fpath,
                          std::optional<const fs::path> gpath) {
  DRL::Program prog;
  Shader vshader(GL_VERTEX_SHADER, vpath);
  prog.attach_shaders({vshader});
  if (fpath.has_value()) {
    Shader fshader(GL_FRAGMENT_SHADER, *fpath);
    prog.attach_shaders({fshader});
  }

  if (gpath.has_value()) {
    Shader gshader(GL_GEOMETRY_SHADER, *gpath);
    prog.attach_shaders({gshader});
  }
  prog.link();
  return prog;
}

//
// Created by zhong on 2021/4/23.
//

#include "shader.hh"
#include "program.hh"
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
using namespace DRL;

// will process #include flag
static std::string LoadShaderFromFileNonStandard(const fs::path &path,
                                                 bool top_level = true) {
  std::string code{};
  std::ifstream file_stream(path);
  if (!file_stream.is_open()) {
    spdlog::error("{} not found!", path.string());
    std::quick_exit(-1);
  }

  std::string lineBuffer;
  const static std::string include_token = R"(#include)";
  auto file_dir_path = path.parent_path();
  while (std::getline(file_stream, lineBuffer)) {
    if (lineBuffer.find(include_token) != std::string::npos) {
      std::string found_path;
      auto start_position = lineBuffer.find_first_of('\"');
      ++start_position;
      auto end_position = lineBuffer.find_last_of('\"');
      found_path =
          lineBuffer.substr(start_position, end_position - start_position);
      auto include_path = canonical((file_dir_path / found_path));
      AssertLog(include_path != path, "{} self included!",
                include_path.string());
      code += LoadShaderFromFileNonStandard(include_path, false);
      continue;
    } else if (lineBuffer.find("#version") != std::string::npos && !top_level) {
      // in included glsl shader, we ignore the #version line
      continue;
    }
    code += lineBuffer + '\n';
  }
  return code;
}
static std::string LoadShaderFromFile(const fs::path &path) {
  std::string code;
  std::ifstream file_stream(path);
  if (!file_stream.is_open()) {
    spdlog::error("{} not found!", path.string());
    std::quick_exit(-1);
  }
  std::stringstream ss;
  ss << file_stream.rdbuf();
  code = ss.str();
  return code;
}
Shader::Shader(GLenum type, const std::string &content, bool compile)
    : content_(content), obj_(type), type_(MapGLEnumToShaderType(type)) {
  if (compile)
    this->compile();
}
Shader::Shader(GLenum type, const fs::path &path, bool compile)
    : content_(LoadShaderFromFile(path)), obj_(type),
      type_(MapGLEnumToShaderType(type)) {
#ifndef NDEBUG
  std::vector<std::string> vsuffix{".vert", ".vs"};
  std::vector<std::string> fsuffix{".frag", ".fs"};
  std::vector<std::string> gsuffix{".geom", ".gs"};
  auto check_inside = [&path](const std::string &suffix,
                              const std::vector<std::string> &list) {
    if (std::find(list.begin(), list.end(), suffix) == list.end()) {
      std::string expected = "[";
      for (const auto &item : list) {
        expected += item;
        expected += ',';
      }
      expected.pop_back();
      expected += ']';
      spdlog::warn(
          "Unknown shader suffix. Path is {}, suffix is {}, expected is {}",
          path.string(), suffix, expected);
    }
  };
  //... and some other rare shader suffixes
  if (!path.has_extension()) {
    spdlog::warn("Shader {}, path {} has no suffix!", obj_.handle(),
                 path.string());
  } else {
    auto suffix = path.extension().string();
    switch (type) {
    case (GL_VERTEX_SHADER):
      check_inside(suffix, vsuffix);
      break;
    case (GL_FRAGMENT_SHADER):
      check_inside(suffix, fsuffix);
      break;
    case (GL_GEOMETRY_SHADER):
      check_inside(suffix, gsuffix);
      break;
    default:
      spdlog::debug("Rare shader! Path is {}", path.string());
    }
  }
#endif
  if (compile)
    this->compile();
}
void Shader::compile() {
  AssertLog(!compiled_, "Shader {} compiled multiple times!", obj_.handle());
  auto content_ptr = content_.c_str();
  glShaderSource(obj_, 1, &content_ptr, nullptr);
  glCompileShader(obj_);
  GLint status;
  glGetShaderiv(obj_, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint maxLength = 0;
    glGetShaderiv(obj_, GL_INFO_LOG_LENGTH, &maxLength);
    // The maxLength includes the NULL character
    std::string infoLog(maxLength, '0');
    glGetShaderInfoLog(obj_, maxLength, nullptr, infoLog.data());
    spdlog::error("{} SHADER_COMPILATION_ERROR: {}", obj_, infoLog);
    spdlog::shutdown();
    std::quick_exit(-1);
  }
  compiled_ = true;
}

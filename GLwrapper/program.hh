//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_PROGRAM_HH
#define DIRENDERLAB_PROGRAM_HH

#include "globject.hh"
#include "shader.hh"
#include <bitset>
#include <glm/glm.hpp>
#include <optional>
#include <variant>
namespace DRL {
class Texture;
class Program {
private:
  // vertex, geometry, frag shaders
  // A bits field to check whether the shaders are correctly attached
  std::bitset<3> shaders_bits_;

protected:
  ProgramObj obj_;
  bool linked_ = false;

#ifdef GL_ARB_BINDLESS
  using Uniform_t =
      std::variant<bool, int, unsigned int, float, glm::mat3, glm::mat4,
                   glm::vec2, glm::vec3, glm::vec4, GLuint64>;
#else
  using Uniform_t = std::variant<bool, int, unsigned int, float, glm::mat3,
                                 glm::mat4, glm::vec2, glm::vec3, glm::vec4>;
#endif

public:
  static Program *current_using_program;
  ~Program() {
    AssertLog(linked_, "Program {} is never linked!", obj_);
    // just warning. Because when the process is ending we don't need to unbind
    // current program.
    AssertWarning(current_using_program != this,
                  "Program {} is using when destroying!", obj_);
  }
  operator GLuint() const { return obj_.handle(); }
  [[nodiscard]] GLuint handle() const { return obj_.handle(); }
  Program(Program &&other) = default;
  Program &operator=(Program &&) = default;
  Program() = default;
  void attach_shaders(
      std::initializer_list<std::reference_wrapper<const Shader>> shaders);

  Program(std::initializer_list<std::reference_wrapper<const Shader>> shaders);
  void link();
  [[nodiscard]] bool linked() const { return linked_; }
  [[nodiscard]] bool isBounded() const { return current_using_program == this; }
  void bind() {
    AssertLog(linked(), "Program {} uses before linking!", obj_);
    // this condition maybe too tough.
    // we need a use_guard<Program> just like std::lock_guard<std::mutex>
    // AssertWarning(!used_, "Program {} is using!", obj_);
    if (current_using_program != this) {
      glUseProgram(obj_);
      current_using_program = this;
    }
  }
  void unbind() {
    AssertLog(isBounded(), "Program {} is not using!", obj_);
    current_using_program = nullptr;
    glUseProgram(0);
  }
  void set_uniform(std::string_view name, const Uniform_t &value) const;
  // TODO: unused interface
  //  void set_uniform(GLuint loc, const Uniform_t &value) const;
};
Program make_program(const fs::path &vpath,
                     std::optional<const fs::path> fpath = std::nullopt,
                     std::optional<const fs::path> gpath = std::nullopt);
} // namespace DRL

#endif // DIRENDERLAB_PROGRAM_HH

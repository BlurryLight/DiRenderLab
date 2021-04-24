//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_PROGRAM_HH
#define DIRENDERLAB_PROGRAM_HH

#include "globject.hh"
#include "shader.hh"
#include <bitset>
#include <glm/glm.hpp>
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
        bool used_ = false;

        using Uniform_t = std::variant<
                bool, int, unsigned int, float,
                glm::mat3, glm::mat4, glm::vec2, glm::vec3, glm::vec4>;

    public:
        ~Program() {
            AssertLog(linked_, "Program {} is never linked!", obj_);
            // just warning. Because when the process is ending we don't need to unbind current program.
            AssertWarning(!used_, "Program {} is using when destroying!", obj_);
        }
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Program(Program &&other) = default;
        Program &operator=(Program &&) = default;
        Program() = default;
        void attach_shaders(std::initializer_list<std::reference_wrapper<const Shader>> shaders);

        Program(std::initializer_list<std::reference_wrapper<const Shader>> shaders);
        void link();
        [[nodiscard]] bool linked() const { return linked_; }
        [[nodiscard]] bool isBounded() const { return used_; }
        void use() {
            AssertLog(linked(), "Program {} uses before linking!", obj_);
            // this condition maybe too tough.
            // we need a use_guard<Program> just like std::lock_guard<std::mutex>
            // AssertWarning(!used_, "Program {} is using!", obj_);
            glUseProgram(obj_);
            used_ = true;
        }
        void unuse() {
            AssertLog(used_, "Program {} is not using!", obj_);
            glUseProgram(0);
            used_ = false;
        }
        void set_uniform(const std::string_view name, const Uniform_t &value) const;
    };
}// namespace DRL


#endif//DIRENDERLAB_PROGRAM_HH

//
// Created by zhong on 2021/4/24.
//

#ifndef DIRENDERLAB_PROGRAM_HH
#define DIRENDERLAB_PROGRAM_HH

#include "globject.hh"
#include "shader.hh"
#include <bitset>
namespace DRL {
    /*
    class Shader1 {
    private:
        std::string content_{};

    protected:
        // a flag need to be checked before using
        bool compiled_ = false;
        ShaderObj obj_;//RAII handle
        GLenum type_;

    public:
        ~Shader1() {
            // It maybe a bug that a shader was created but never compiled!
            AssertLog(compiled_, "Shader {} is never used!");
        }
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Shader1(Shader1 &&other) = default;
        Shader1 &operator=(Shader1 &&) = default;
        // the default behaviour is compile the shader in constructor
        Shader1(GLenum type, const std::string &content, bool compile = true);
        Shader1(GLenum type, const fs::path &path, bool compile = true);
        void compile() const;
    };
    */
    class Program {
    private:
        // vertex, geometry, frag shaders
        // A bits field to check whether the shaders are correctly attached
        std::bitset<3> shaders_bits_;

    protected:
        ProgramObj obj_;
        bool linked_ = false;

    public:
        ~Program() {
            AssertLog(linked_, "Program {} is never linked!");
        }
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Program(Program &&other) = default;
        Program &operator=(Program &&) = default;
        Program() = default;
        void attach_shaders(std::initializer_list<std::reference_wrapper<const Shader1>> shaders);

        Program(std::initializer_list<std::reference_wrapper<const Shader1>> shaders);
        void link();
        [[nodiscard]] bool linked() const { return linked_; }
        void use() { glUseProgram(obj_); }
    };
}// namespace DRL


#endif//DIRENDERLAB_PROGRAM_HH

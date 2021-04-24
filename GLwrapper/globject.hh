//
// Created by zhong on 2021/4/23.
//

#ifndef DIRENDERLAB_GLOBJECT_HH
#define DIRENDERLAB_GLOBJECT_HH

#include "global.hh"
#include <cassert>
#include <glad/glad.h>
//basic RAII class to manage all OpenGL handle
namespace DRL {

    class GLObject {
    protected:
        GLuint handle_ = 0;

    public:
        operator GLuint() const {
            return handle_;
        }

        // no default construction is allowed!
        explicit GLObject(GLuint handle) {
            AssertLog(handle, "Handle should be valid value!");
            handle_ = handle;
        }
        // nor copy is allowed
        GLObject(const GLObject &) = delete;
        GLObject &operator=(const GLObject &) = delete;
        // but it is movable
        GLObject(GLObject &&other) noexcept {
            handle_ = other.handle_;
            other.handle_ = 0;
        }

        GLObject &operator=(GLObject &&other) noexcept {
            handle_ = other.handle_;
            other.handle_ = 0;
            return *this;
        }
        virtual ~GLObject() = default;
        [[nodiscard]] GLuint handle() const { return handle_; }
    };

    class ShaderObj : public GLObject {
    public:
        explicit ShaderObj(GLenum type) : GLObject(glCreateShader(type)) {
        }
        ShaderObj(ShaderObj &&other) noexcept : GLObject(std::move(other)) {}
        ShaderObj &operator=(ShaderObj &&other) noexcept {
            GLObject::operator=(std::move(other));
            return *this;
        }
        ~ShaderObj() override {
            if (handle()) {
                spdlog::warn("RAII is destroying {} Shader handle! Be cautious!", handle());
                glDeleteShader(handle());
                handle_ = 0;
            }
        }
    };

    class ProgramObj : public GLObject {
    public:
        ProgramObj() : GLObject(glCreateProgram()) {
        }
        ~ProgramObj() override {
            if (handle()) {
                spdlog::warn("RAII is destroying {} Program handle! Be cautious!", handle());
                glDeleteProgram(handle());
                handle_ = 0;
            }
        }
        ProgramObj(ProgramObj &&other) noexcept : GLObject(std::move(other)) {}
        ProgramObj &operator=(ProgramObj &&other) noexcept {
            GLObject::operator=(std::move(other));
            return *this;
        }
    };
}// namespace DRL

#endif//DIRENDERLAB_GLOBJECT_HH

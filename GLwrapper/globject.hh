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
    private:
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
        ShaderObj(ShaderObj &&) = default;
        ShaderObj &operator=(ShaderObj &&) = default;
        ~ShaderObj() override {
            glDeleteShader(handle());
        }
    };
}// namespace DRL

#endif//DIRENDERLAB_GLOBJECT_HH

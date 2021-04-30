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

        GLObject() = default;
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
        ~GLObject() = default;
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
        ~ShaderObj() {
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
        ~ProgramObj() {
            if (handle()) {
                spdlog::warn("RAII is destroying {} Program handle! Be cautious!", handle());
                // There is a strange error here.
                // glGetError will report it is a invalid operation.
                // According to the latest OpenGL 4.5 reference, glDeleteProgram should never return GL_INVALID_OPERATION.
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
    class VertexBufferObject : public GLObject {
    public:
        VertexBufferObject() : GLObject() {
            glCreateBuffers(1, &handle_);
        }
        ~VertexBufferObject() {
            if (handle()) {
                glDeleteBuffers(1, &handle_);
                handle_ = 0;
            }
        }
        VertexBufferObject(VertexBufferObject &&other) noexcept : GLObject(std::move(other)) {}
        VertexBufferObject &operator=(VertexBufferObject &&other) noexcept {
            GLObject::operator=(std::move(other));
            return *this;
        }
    };

    class VertexArrayObject : public GLObject {
    public:
        VertexArrayObject() : GLObject() {
            glCreateVertexArrays(1, &handle_);
        }
        ~VertexArrayObject() {
            if (handle()) {
                glDeleteVertexArrays(1, &handle_);
                handle_ = 0;
            }
        }
        VertexArrayObject(VertexArrayObject &&other) noexcept : GLObject(std::move(other)) {}
        VertexArrayObject &operator=(VertexArrayObject &&other) noexcept {
            GLObject::operator=(std::move(other));
            return *this;
        }
    };
    class TextureObject : public GLObject {
    public:
        explicit TextureObject(GLenum textureType) : GLObject() {
            glCreateTextures(textureType, 1, &handle_);
        }
        ~TextureObject() {
            if (handle()) {
                glDeleteTextures(1, &handle_);
                handle_ = 0;
            }
        }
        TextureObject(TextureObject &&other) noexcept : GLObject(std::move(other)) {}
        TextureObject &operator=(TextureObject &&other) noexcept {
            GLObject::operator=(std::move(other));
            return *this;
        }
    };

    class FramebufferObj : public GLObject {
    public:
        FramebufferObj() : GLObject() {
            glCreateFramebuffers(1, &handle_);
        }
        ~FramebufferObj() {
            if (handle()) {
                glDeleteFramebuffers(1, &handle_);
                handle_ = 0;
            }
        }
        FramebufferObj(FramebufferObj &&other) noexcept : GLObject(std::move(other)) {}
        FramebufferObj &operator=(FramebufferObj &&other) noexcept {
            GLObject::operator=(std::move(other));
            return *this;
        }
    };
}// namespace DRL

#endif//DIRENDERLAB_GLOBJECT_HH

//
// Created by zhong on 2021/4/23.
//

#ifndef DIRENDERLAB_SHADER_HH
#define DIRENDERLAB_SHADER_HH

#include "globject.hh"
#include "utils/resource_path_searcher.h"// for path searcher
#include <glm/glm.hpp>
#include <string>
namespace DRL {


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
        void compile() const {
            AssertLog(!compiled_, "Shader {} compiled multiple times!", obj_.handle());
            auto content_ptr = content_.c_str();
            glShaderSource(obj_, 1, &content_ptr, nullptr);
            glCompileShader(obj_);
            GLint status;
            glGetShaderiv(obj_, GL_COMPILE_STATUS, &status);
            if (status != GL_TRUE) {
                GLchar infoLog[1024];
                glGetShaderInfoLog(obj_, 1024, nullptr, infoLog);
                spdlog::error("{} SHADER_COMPILATION_ERROR: {}", obj_, infoLog);
                spdlog::shutdown();
                std::abort();
            }
        }
    };

    // A container has shader string
    //    class ShaderContent
    class Shader {
    public:
        unsigned int ID;
        // constructor generates the shader on the fly
        // ------------------------------------------------------------------------
        Shader(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr);
        // activate the shader
        // ------------------------------------------------------------------------
        void use();
        // utility uniform functions
        // ------------------------------------------------------------------------
        void setBool(const std::string &name, bool value) const;
        // ------------------------------------------------------------------------
        void setInt(const std::string &name, int value) const;
        // ------------------------------------------------------------------------
        void setFloat(const std::string &name, float value) const;
        // ------------------------------------------------------------------------
        void setVec2(const std::string &name, const glm::vec2 &value) const;
        void setVec2(const std::string &name, float x, float y) const;
        // ------------------------------------------------------------------------
        void setVec3(const std::string &name, const glm::vec3 &value) const;
        void setVec3(const std::string &name, float x, float y, float z) const;
        // ------------------------------------------------------------------------
        void setVec4(const std::string &name, const glm::vec4 &value) const;
        void setVec4(const std::string &name, float x, float y, float z, float w);
        // ------------------------------------------------------------------------
        void setMat2(const std::string &name, const glm::mat2 &mat) const;
        // ------------------------------------------------------------------------
        void setMat3(const std::string &name, const glm::mat3 &mat) const;
        // ------------------------------------------------------------------------
        void setMat4(const std::string &name, const glm::mat4 &mat) const;

    private:
        // utility function for checking shader compilation/linking errors.
        // ------------------------------------------------------------------------
        void checkCompileErrors(GLuint shader, std::string type);
    };
}// namespace DRL


#endif//DIRENDERLAB_SHADER_HH

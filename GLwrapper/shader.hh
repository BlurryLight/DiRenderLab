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

    //current we only support v,g,f shader
    enum ShaderType { kVShader = 0,
                      kGShader = 1,
                      kFShader = 2,
                      kAdvancedShader = 3 };
    inline ShaderType MapGLEnumToShaderType(GLenum t) {
        if (t == GL_VERTEX_SHADER) return kVShader;
        else if (t == GL_FRAGMENT_SHADER)
            return kFShader;
        else if (t == GL_GEOMETRY_SHADER)
            return kGShader;
        else
            return kAdvancedShader;
    }
    inline std::string MapShaderTypeToString(ShaderType t) {
        switch (t) {
            case kVShader:
                return {"VShader"};
            case kFShader:
                return {"FShader"};
            case kGShader:
                return {"GShader"};
            case kAdvancedShader:
                return {"AdvancedShader"};
        }
    }

    class Shader1 {
    private:
        std::string content_{};

    protected:
        bool compiled_ = false;
        // a flag need to be checked before using
        ShaderObj obj_;//RAII handle
        ShaderType type_;

    public:
        ~Shader1() {
            // It maybe a bug that a shader was created but never compiled!
            // check it is compiled or be moved
            AssertLog(compiled_ || (obj_.handle() == 0), "Shader {} is never compiled!");
        }
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Shader1(Shader1 &&other) = default;
        Shader1 &operator=(Shader1 &&) = default;
        // the default behaviour is compile the shader in constructor
        Shader1(GLenum type, const std::string &content, bool compile = true);
        Shader1(GLenum type, const fs::path &path, bool compile = true);
        void compile();
        [[nodiscard]] bool compiled() const { return compiled_; }
        [[nodiscard]] ShaderType type() const { return type_; }
    };

    // A container has shader string
    class Program;
    class Shader {
    public:
        unsigned int ID;
        Program *program;
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

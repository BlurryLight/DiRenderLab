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
        return "Unknown shader";
    }

    class Shader {
    private:
        std::string content_{};

    protected:
        bool compiled_ = false;
        // a flag need to be checked before using
        ShaderObj obj_;//RAII handle
        ShaderType type_;

    public:
        ~Shader() {
            // It maybe a bug that a shader was created but never compiled!
            // check it is compiled or be moved
            AssertLog(compiled_ || (obj_.handle() == 0), "Shader {} is never compiled!");
        }
        operator GLuint() const { return obj_.handle(); }
        [[nodiscard]] GLuint handle() const { return obj_.handle(); }
        Shader(Shader &&other) = default;
        Shader &operator=(Shader &&) = default;
        // the default behaviour is compile the shader in constructor
        Shader(GLenum type, const std::string &content, bool compile = true);
        Shader(GLenum type, const fs::path &path, bool compile = true);
        void compile();
        [[nodiscard]] bool compiled() const { return compiled_; }
        [[nodiscard]] ShaderType type() const { return type_; }
    };

}// namespace DRL


#endif//DIRENDERLAB_SHADER_HH

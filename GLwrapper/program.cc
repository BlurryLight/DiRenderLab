//
// Created by zhong on 2021/4/24.
//

#include "program.hh"
using namespace DRL;
void Program::link() {
    AssertLog(!linked_, "Program {} linked multiple times!", obj_.handle());
    if (!(shaders_bits_[0] && shaders_bits_[2]))// check vshader and fshader
    {
        spdlog::warn("Be cautious! Program {} lacks vshader or fshader!", obj_);
    }
    glLinkProgram(obj_);
    GLint status;
    glGetProgramiv(obj_, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint maxLength = 0;
        glGetProgramiv(obj_, GL_INFO_LOG_LENGTH, &maxLength);
        // The maxLength includes the NULL character
        std::string infoLog(maxLength, '0');
        glGetProgramInfoLog(obj_, maxLength, nullptr, infoLog.data());
        spdlog::error("{} PROGRAM_LINK_ERROR: {}", obj_, infoLog);
        spdlog::shutdown();
        std::abort();
    }
    linked_ = true;
}
Program::Program(std::initializer_list<std::reference_wrapper<const Shader1>> shaders) {
    this->attach_shaders(shaders);
}
void Program::attach_shaders(std::initializer_list<std::reference_wrapper<const Shader1>> shaders) {
    // shader is a const reference
    AssertLog(!linked_, "AttachShader on an linked program!");
    for (auto shader : shaders) {
        if (!shader.get().compiled()) {
            spdlog::warn("Shader {} is not compiled when attaching!", shader.get().handle());
        }
        auto check_bits = [this](ShaderType t) {
            AssertLog(!this->shaders_bits_[t], "{} Shader is attached by multiple times!", MapShaderTypeToString(t));
        };
        auto type = shader.get().type();
        auto shader_handle = shader.get().handle();
        if (type == kVShader || type == kFShader || type == kGShader) {
            check_bits(type);
            shaders_bits_[type] = true;
        }
        glAttachShader(obj_, shader_handle);
    }
}

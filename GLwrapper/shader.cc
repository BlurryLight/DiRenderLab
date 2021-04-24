//
// Created by zhong on 2021/4/23.
//

#include "shader.hh"
#include "program.hh"
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
using namespace DRL;

static std::string LoadShaderFromFile(const fs::path &path) {
    std::string code;
    std::ifstream file_stream(path);
    if (!file_stream.is_open()) {
        spdlog::error("{} not found!", path.string());
        std::abort();
    }
    std::stringstream ss;
    ss << file_stream.rdbuf();
    code = ss.str();
    return code;
}
static std::string LoadShaderFromFile(const std::string &path) {
    std::string code;
    std::ifstream file_stream(path);
    if (!file_stream.is_open()) {
        spdlog::error("{} not found!", path);
        std::abort();
    }
    std::stringstream ss;
    ss << file_stream.rdbuf();
    code = ss.str();
    return code;
}
DRL::Shader::Shader(const char *vertexPath, const char *fragmentPath, const char *geometryPath) {
    auto vshader = Shader1(GL_VERTEX_SHADER, fs::path(vertexPath));
    auto fshader = Shader1(GL_FRAGMENT_SHADER, fs::path(fragmentPath));
    ID = glCreateProgram();
    if (geometryPath != nullptr) {
        auto gshader = Shader1(GL_GEOMETRY_SHADER, fs::path(geometryPath));
        gshader.compile();
        glAttachShader(ID, gshader);
    }
    program = new Program({vshader, fshader});
    program->link();
    ID = program->handle();
    ////    glAttachShader(ID, vshader);
    ////    glAttachShader(ID, fshader);
    ////    glLinkProgram(ID);
    //    checkCompileErrors(ID, "PROGRAM");
}
void Shader::use() {
    glUseProgram(ID);
}
void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int) value);
}
void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setVec2(const std::string &name, const glm::vec2 &value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec2(const std::string &name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}
void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setVec4(const std::string &name, float x, float y, float z, float w) {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}
void Shader::setMat2(const std::string &name, const glm::mat2 &mat) const {
    glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::setMat3(const std::string &name, const glm::mat3 &mat) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}
Shader1::Shader1(GLenum type, const std::string &content, bool compile)
    : obj_(type), content_(content), type_(MapGLEnumToShaderType(type)) {
    if (compile)
        this->compile();
}
Shader1::Shader1(GLenum type, const fs::path &path, bool compile)
    : obj_(type), content_(LoadShaderFromFile(path)), type_(MapGLEnumToShaderType(type)) {
#ifndef NDEBUG
    std::vector<std::string> vsuffix{".vert", ".vs"};
    std::vector<std::string> fsuffix{".frag", ".fs"};
    std::vector<std::string> gsuffix{".geom", ".gs"};
    auto check_inside = [&path](const std::string &suffix, const std::vector<std::string> &list) {
        if (std::find(list.begin(), list.end(), suffix) == list.end()) {
            std::string expected = "[";
            for (const auto &item : list) {
                expected += item;
                expected += ',';
            }
            expected.pop_back();
            expected += ']';
            spdlog::warn("Unknown shader suffix. Path is {}, suffix is {}, expected is {}", path.string(), suffix, expected);
        }
    };
    //... and some other rare shader suffixes
    if (!path.has_extension()) {
        spdlog::warn("Shader {}, path {} has no suffix!", obj_.handle(), path.string());
    } else {
        auto suffix = path.extension().string();
        switch (type) {
            case (GL_VERTEX_SHADER):
                check_inside(suffix, vsuffix);
                break;
            case (GL_FRAGMENT_SHADER):
                check_inside(suffix, fsuffix);
                break;
            case (GL_GEOMETRY_SHADER):
                check_inside(suffix, gsuffix);
                break;
            default:
                spdlog::info("Rare shader! Path is {}", path.string());
        }
    }
#endif
    if (compile)
        this->compile();
}
void Shader1::compile() {
    AssertLog(!compiled_, "Shader {} compiled multiple times!", obj_.handle());
    auto content_ptr = content_.c_str();
    glShaderSource(obj_, 1, &content_ptr, nullptr);
    glCompileShader(obj_);
    GLint status;
    glGetShaderiv(obj_, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint maxLength = 0;
        glGetShaderiv(obj_, GL_INFO_LOG_LENGTH, &maxLength);
        // The maxLength includes the NULL character
        std::string infoLog(maxLength, '0');
        glGetShaderInfoLog(obj_, maxLength, nullptr, infoLog.data());
        spdlog::error("{} SHADER_COMPILATION_ERROR: {}", obj_, infoLog);
        spdlog::shutdown();
        std::abort();
    }
    compiled_ = true;
}

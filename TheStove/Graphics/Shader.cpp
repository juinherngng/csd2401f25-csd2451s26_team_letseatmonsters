#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const std::string & vertexFile, const std::string & fragmentFile) {
    std::string vertexSource = ReadFile(vertexFile);
    std::string fragmentSource = ReadFile(fragmentFile);

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    programID = LinkProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    InitUniforms();
}

Shader::~Shader() {
    glDeleteProgram(programID);
}

std::string Shader::ReadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Shader::CompileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Error compiling shader: " << infoLog << std::endl;
    }
    return shader;
}

GLuint Shader::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Error linking program: " << infoLog << std::endl;
    }
    return program;
}

void Shader::InitUniforms() {
    // Cache uniform locations
    uniformModelMatrix = glGetUniformLocation(programID, "u_Model");
    uniformViewMatrix = glGetUniformLocation(programID, "u_View");
    uniformProjMatrix = glGetUniformLocation(programID, "u_Projection");
}

void Shader::Use() const {
    glUseProgram(programID);
}

void Shader::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(uniformModelMatrix, 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(uniformViewMatrix, 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetProjectionMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(uniformProjMatrix, 1, GL_FALSE, &mat[0][0]);
}

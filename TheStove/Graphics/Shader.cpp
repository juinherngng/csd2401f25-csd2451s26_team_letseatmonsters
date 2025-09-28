#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    // Debug uniform locations
    /*std::cout << "u_Model location: " << uniformModelMatrix << std::endl;
    std::cout << "u_View location: " << uniformViewMatrix << std::endl;
    std::cout << "u_Projection location: " << uniformProjMatrix << std::endl*/;

    if (uniformModelMatrix == -1 || uniformViewMatrix == -1 || uniformProjMatrix == -1) {
        std::cerr << "ERROR: One or more uniform locations are invalid!" << std::endl;
    }

}

void Shader::Use() const {
    //std::cout << "Attempting to bind program ID: " << programID << std::endl;
    
    // Check if program exists and is valid
    GLboolean isProgram = glIsProgram(programID);
    //std::cout << "glIsProgram(" << programID << ") = " << (isProgram ? "true" : "false") << std::endl;
    
    glUseProgram(programID);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "glUseProgram error: " << error << std::endl;
    }
    
    // Verify binding worked
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    //std::cout << "Current program after glUseProgram: " << currentProgram << std::endl;
}

void Shader::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(uniformModelMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(uniformViewMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetProjectionMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(uniformProjMatrix, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetTexture(const std::string& name, int textureUnit) const {
    GLint location = glGetUniformLocation(programID, name.c_str());
    if (location != -1) {
        glUniform1i(location, textureUnit);
    }
}

void Shader::SetColorTint(const glm::vec4& color) const {
    GLint location = glGetUniformLocation(programID, "u_ColorTint");
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(color));
    }
}


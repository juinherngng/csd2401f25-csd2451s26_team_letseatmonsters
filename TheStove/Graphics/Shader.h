#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>


class Shader {
public:
    Shader(const std::string& vertexFile, const std::string& fragmentFile);
    ~Shader();

    void Use() const;

    // Set uniforms by cached location
    void SetModelMatrix(const glm::mat4& mat) const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjectionMatrix(const glm::mat4& mat) const;
    void SetTexture(const std::string& name, int textureUnit) const;
    void SetColorTint(const glm::vec4& color) const;  // For tinting textures

private:
    GLuint programID;

    // Cached uniform locations for performance
    GLint uniformModelMatrix;
    GLint uniformViewMatrix;
    GLint uniformProjMatrix;

    void InitUniforms();

    std::string ReadFile(const std::string& filepath);
    GLuint CompileShader(GLenum type, const std::string& source);
    GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
};

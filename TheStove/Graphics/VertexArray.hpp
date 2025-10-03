#pragma once
#include <glad/glad.h>
#include "VertexBuffer.hpp"

class VertexArray {
    GLuint ID;
public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void Unbind() const;
    void AddBuffer(const VertexBuffer& vb, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
};

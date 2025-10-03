#pragma once
#include <glad/glad.h>

class VertexBuffer {
    GLuint ID;
public:
    VertexBuffer(const void* data, size_t size);
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;
};
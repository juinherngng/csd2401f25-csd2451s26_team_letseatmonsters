#include "VertexArray.h"
#include <iostream>

VertexArray::VertexArray() {
    glGenVertexArrays(1, &ID);
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &ID);
}

void VertexArray::Bind() const {
    glBindVertexArray(ID);
}

void VertexArray::Unbind() const {
    glBindVertexArray(0);
}

void VertexArray::AddBuffer(const VertexBuffer& vb, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    Bind();
    vb.Bind();
    /*std::cout << "Setting up vertex attribute " << index
        << ", size=" << size
        << ", stride=" << stride
        << ", offset=" << (uintptr_t)pointer << std::endl;*/

    glVertexAttribPointer(index, size, type, normalized, stride, pointer);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error in glVertexAttribPointer: " << error << std::endl;
        return; // Don't enable if there was an error
    }

    glEnableVertexAttribArray(index);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error in glEnableVertexAttribArray: " << error << std::endl;
    }
}
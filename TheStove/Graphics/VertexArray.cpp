#include "VertexArray.h"

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
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    glEnableVertexAttribArray(index);
}
#include "Mesh.h"

Mesh::Mesh(const float* vertices, size_t vertexCount, size_t vertexSize)
    : vbo(vertices, vertexCount* vertexSize), vertexCount(vertexCount) {
    // Setup VAO attribute pointers depending on vertex layout, for example:
    vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0); // position
    vao.AddBuffer(vbo, 1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float))); // color
}

void Mesh::Draw() const {
    vao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);
    vao.Unbind();
}
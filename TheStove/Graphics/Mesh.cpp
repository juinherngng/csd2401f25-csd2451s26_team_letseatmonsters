#include "Mesh.h"
#include <iostream>

Mesh::Mesh(const float* vertices, size_t vertexCount, size_t vertexSize, VertexLayout layout)
    : vao(), vbo(vertices, vertexCount* vertexSize), vertexCount(vertexCount) {

    switch (layout) {
    case POSITION_COLOR:
        // Original triangle setup: pos(3) + color(3)
        vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);                        // position
        vao.AddBuffer(vbo, 1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));     // color
        break;

    case POSITION_TEXTURE:
        // Textured sprite setup: pos(3) + texcoord(2)
        vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);                        // position
        vao.AddBuffer(vbo, 1, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));     // texture coords
        break;
    }
}

void Mesh::Draw() const {
    vao.Bind();
    // Check for errors before drawing
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after VAO bind: " << error << std::endl;
    }

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error after glDrawArrays: " << error << std::endl;
    }

    vao.Unbind();
}

void Mesh::Draw(const Texture* texture) const {
    vao.Bind();

    if (texture) {
        texture->Bind(0);  // Bind texture to slot 0
    }

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

    if (texture) {
        texture->Unbind();
    }

    vao.Unbind();
}
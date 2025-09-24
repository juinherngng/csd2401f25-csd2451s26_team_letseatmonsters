#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"

class Mesh {
    VertexArray vao;
    VertexBuffer vbo;
    size_t vertexCount;

public:
    Mesh(const float* vertices, size_t vertexCount, size_t vertexSize);
    void Draw() const;
};
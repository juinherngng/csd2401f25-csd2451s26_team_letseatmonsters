#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "Texture.h"

class Texture;

class Mesh {

public:

    enum VertexLayout{
        POSITION_COLOR,    // position (3) + color (3) = 6 floats
        POSITION_TEXTURE   // position (3) + texcoord (2) = 5 floats
    };

    Mesh(const float* vertices, size_t vertexCount, size_t vertexSize, VertexLayout layout = POSITION_COLOR);

    void Draw() const;
    void Draw(const Texture* texture) const;

private:
    VertexArray vao;
    VertexBuffer vbo;
    size_t vertexCount;
};
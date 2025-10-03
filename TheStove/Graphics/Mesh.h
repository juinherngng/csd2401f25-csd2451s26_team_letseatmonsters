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

	// used GLsizei instead of size_t for vertexSize - juinherng
    Mesh(const float* vertices, GLsizei vertexCount, GLsizei vertexSize, VertexLayout layout = POSITION_COLOR);

    void Draw() const;
    void Draw(const Texture* texture) const;

private:
    VertexArray vao;
    VertexBuffer vbo;
    // conversion from size_t to GLsizei warning
    //size_t vertexCount;
    // i used this instead - juinherng
	GLsizei vertexCount;
};
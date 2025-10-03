/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Mesh.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu

DESCRIPTION:		Lightweight wrapper for a VAO + VBO with fixed vertex layouts and draw helpers.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "Texture.hpp"

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
	GLsizei vertexCount;
};
/*
----------------------------------------------------------------------------------------------------
FILE NAME:			VertexBuffer.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu

DESCRIPTION:		RAII wrapper for a GL Array Buffer storing immutable vertex data.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

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
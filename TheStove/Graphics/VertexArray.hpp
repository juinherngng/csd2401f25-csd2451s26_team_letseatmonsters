/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexArray.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		RAII wrapper around a GL Vertex Array Object; manages attribute binding.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glad/glad.h>

#include "VertexBuffer.hpp"

class VertexArray {
	GLuint ID;
public:
	VertexArray();
	~VertexArray();

	void Bind() const;
	void Unbind() const;
	void AddBuffer(const VertexBuffer& vb, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
};

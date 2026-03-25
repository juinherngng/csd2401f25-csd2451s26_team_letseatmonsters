/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexArray.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		RAII wrapper around a GL Vertex Array Object; manages attribute binding.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "VertexBuffer.hpp"

#include <glad/glad.h>

// RAII wrapper around a GL Vertex Array Object; manages attribute binding.
class VertexArray {
	// OpenGL ID for the VAO
	GLuint ID;
public:
	// Create a new VAO and store its ID.
	VertexArray();
	~VertexArray();

	// Bind the VAO and set up vertex attribute pointers for a given VertexBuffer.
	void Bind() const;
	void Unbind() const;
	void AddBuffer(const VertexBuffer& vb, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
};


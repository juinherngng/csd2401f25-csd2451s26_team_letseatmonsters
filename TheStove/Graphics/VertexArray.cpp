/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexArray.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements VAO creation/destruction, binding, unbinding, and attribute setup.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Core/Logger.hpp"

#include "VertexArray.hpp"

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

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[VertexArray] OpenGL error in glVertexAttribPointer: " << error);
		return; // Don't enable if there was an error
	}

	glEnableVertexAttribArray(index);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[VertexArray] OpenGL error in glEnableVertexAttribArray: " << error);
	}
}

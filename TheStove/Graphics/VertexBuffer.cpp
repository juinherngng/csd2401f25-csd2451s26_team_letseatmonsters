/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexBuffer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements VBO creation, data upload in constructor, bind/unbind, and destruction.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "VertexBuffer.hpp"

VertexBuffer::VertexBuffer(const void* data, size_t size) {
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
	: ID(other.ID) {
	other.ID = 0;
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	if (ID != 0) {
		glDeleteBuffers(1, &ID);
	}

	ID = other.ID;
	other.ID = 0;
	return *this;
}

VertexBuffer::~VertexBuffer() {
	if (ID != 0) {
		glDeleteBuffers(1, &ID);
	}
}

void VertexBuffer::Bind() const {
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VertexBuffer::Unbind() const {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexBuffer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (45%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (55%)

 DESCRIPTION:		Implements VBO creation, data upload in constructor, bind/unbind, and destruction.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineGraphics/VertexBuffer.hpp"

/**
 * @brief Creates a vertex buffer and uploads immutable vertex data into it.
 * @param data Pointer to the vertex data that should be uploaded.
 * @param size Size in bytes of the supplied vertex data.
 */
VertexBuffer::VertexBuffer(const void* data, size_t size) {
	// Allocate a fresh GPU buffer handle for this wrapper.
	glGenBuffers(1, &ID);

	// Bind the new buffer and upload the immutable vertex payload immediately.
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

/**
 * @brief Transfers buffer ownership from another wrapper during construction.
 * @param other Source wrapper whose OpenGL handle should be adopted.
 */
VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
	: ID(other.ID) {
	// Clear the moved-from wrapper so only this instance owns the buffer handle.
	other.ID = 0;
}

/**
 * @brief Transfers buffer ownership from another wrapper during assignment.
 * @param other Source wrapper whose OpenGL handle should be adopted.
 * @return Reference to this wrapper after ownership transfer.
 */
VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
	if (this == &other) {
		// Ignore self-move assignment to avoid releasing the live buffer by mistake.
		return *this;
	}

	if (ID != 0) {
		// Release any buffer already owned by this wrapper before adopting the new one.
		glDeleteBuffers(1, &ID);
	}

	ID = other.ID;
	other.ID = 0;
	return *this;
}

/**
 * @brief Releases the owned OpenGL buffer object.
 */
VertexBuffer::~VertexBuffer() {
	if (ID != 0) {
		// Return the GPU buffer to OpenGL when this wrapper is destroyed.
		glDeleteBuffers(1, &ID);
	}
}

/**
 * @brief Binds this buffer to `GL_ARRAY_BUFFER`.
 */
void VertexBuffer::Bind() const {
	// Make this buffer the active array-buffer source for subsequent vertex setup.
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

/**
 * @brief Unbinds any buffer from `GL_ARRAY_BUFFER`.
 */
void VertexBuffer::Unbind() const {
	// Restore the default array-buffer binding.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

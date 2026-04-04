/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexBuffer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (55%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (45%)

 DESCRIPTION:		RAII wrapper for a GL Array Buffer storing immutable vertex data.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <cstddef>
#include <glad/glad.h>

class VertexBuffer {
	GLuint ID = 0;
public:
	/**
	 * @brief Creates a vertex buffer and uploads immutable vertex data into it.
	 * @param data Pointer to the vertex data that should be uploaded.
	 * @param size Size in bytes of the supplied vertex data.
	 */
	VertexBuffer(const void* data, size_t size);

	/**
	 * @brief Releases the owned OpenGL buffer object.
	 */
	~VertexBuffer();

	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer& operator=(const VertexBuffer&) = delete;

	/**
	 * @brief Transfers buffer ownership from another wrapper during construction.
	 * @param other Source wrapper whose OpenGL handle should be adopted.
	 */
	VertexBuffer(VertexBuffer&& other) noexcept;

	/**
	 * @brief Transfers buffer ownership from another wrapper during assignment.
	 * @param other Source wrapper whose OpenGL handle should be adopted.
	 * @return Reference to this wrapper after ownership transfer.
	 */
	VertexBuffer& operator=(VertexBuffer&& other) noexcept;

	/**
	 * @brief Binds this buffer to `GL_ARRAY_BUFFER`.
	 */
	void Bind() const;

	/**
	 * @brief Unbinds any buffer from `GL_ARRAY_BUFFER`.
	 */
	void Unbind() const;
};

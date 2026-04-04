/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexArray.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (15%)

 DESCRIPTION:		RAII wrapper around a GL Vertex Array Object; manages attribute binding.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glad/glad.h>

#include "EngineGraphics/VertexBuffer.hpp"

class VertexArray {
	GLuint ID;
public:
	/**
	 * @brief Creates a vertex array object and stores its OpenGL handle.
	 */
	VertexArray();

	/**
	 * @brief Releases the owned vertex array object.
	 */
	~VertexArray();

	/**
	 * @brief Binds this vertex array object.
	 */
	void Bind() const;

	/**
	 * @brief Unbinds any active vertex array object.
	 */
	void Unbind() const;

	/**
	 * @brief Configures one vertex attribute binding on this VAO.
	 * @param vb Vertex buffer whose data should feed the attribute.
	 * @param index Attribute slot index to configure.
	 * @param size Number of components per vertex attribute.
	 * @param type OpenGL component type enum.
	 * @param normalized Whether fixed-point data should be normalized.
	 * @param stride Byte stride between consecutive vertices.
	 * @param pointer Byte offset to the first component in the vertex.
	 */
	void AddBuffer(const VertexBuffer& vb, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
};

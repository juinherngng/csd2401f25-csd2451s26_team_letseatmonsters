/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexArray.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (15%)

 DESCRIPTION:		Implements VAO creation/destruction, binding, unbinding, and attribute setup.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/VertexArray.hpp"

/**
 * @brief Creates a vertex array object and stores its OpenGL handle.
 */
VertexArray::VertexArray() {
	// Ask OpenGL for a fresh VAO handle owned by this wrapper.
	glGenVertexArrays(1, &ID);
}

/**
 * @brief Releases the owned vertex array object.
 */
VertexArray::~VertexArray() {
	// Return the VAO handle to OpenGL during wrapper destruction.
	glDeleteVertexArrays(1, &ID);
}

/**
 * @brief Binds this vertex array object.
 */
void VertexArray::Bind() const {
	// Make this VAO the active target for attribute and draw-state operations.
	glBindVertexArray(ID);
}

/**
 * @brief Unbinds any active vertex array object.
 */
void VertexArray::Unbind() const {
	// Restore the default VAO binding.
	glBindVertexArray(0);
}

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
void VertexArray::AddBuffer(const VertexBuffer& vb, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
	// Bind both the VAO and source VBO before describing the attribute layout.
	Bind();
	vb.Bind();

	// Describe how OpenGL should interpret this attribute inside the bound vertex buffer.
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[VertexArray] OpenGL error in glVertexAttribPointer: " << error);
		return;
	}

	// Enable the attribute slot once its layout has been described successfully.
	glEnableVertexAttribArray(index);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		// Surface enable failures while the responsible attribute index is still obvious.
		TS_LOG_ERROR("[VertexArray] OpenGL error in glEnableVertexAttribArray: " << error);
	}
}

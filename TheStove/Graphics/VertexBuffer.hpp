/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			VertexBuffer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		RAII wrapper for a GL Array Buffer storing immutable vertex data.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glad/glad.h>

// Simple RAII wrapper for a GL Array Buffer storing immutable vertex data.
class VertexBuffer {
	// Note: this class is not copyable or movable, as it directly manages a GPU resource.
	GLuint ID;
public:
	// Create a vertex buffer and upload the given data to the GPU. The buffer is immutable (GL_STATIC_DRAW).
	VertexBuffer(const void* data, size_t size);
	~VertexBuffer();

	// Bind the buffer to GL_ARRAY_BUFFER for use in vertex attribute setup and drawing.
	void Bind() const;
	void Unbind() const;
};

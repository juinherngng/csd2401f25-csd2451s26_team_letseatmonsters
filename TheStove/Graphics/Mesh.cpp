/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Mesh.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Binds vertex attributes for supported layouts and issues glDrawArrays calls.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <iostream>

#include "Mesh.hpp"

// Constructs a mesh by uploading vertex data and configuring a VAO
Mesh::Mesh(const float* vertices, GLsizei vertexCount, GLsizei vertexSize, VertexLayout layout)
	: vao(), vbo(vertices, vertexCount* vertexSize), vertexCount(static_cast<GLsizei>(vertexCount)) {

	switch (layout) {
		case POSITION_COLOR:
		// Original triangle setup: pos(3) + color(3)
		vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);                        // position
		vao.AddBuffer(vbo, 1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));     // color
		break;

		case POSITION_TEXTURE:
		// Textured sprite setup: pos(3) + texcoord(2)
		vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);                        // position
		vao.AddBuffer(vbo, 1, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));     // texture coords
		break;
	}
}

// Drawsthe mesh as GL_TRIANGLES using the configured VAO
void Mesh::Draw() const {
	vao.Bind();
	// Check for errors before drawing
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after VAO bind: " << error << std::endl;
	}

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

	error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after glDrawArrays: " << error << std::endl;
	}

	vao.Unbind();
}

// Draw the mesh with an optional bound texture on texture unit 0.
void Mesh::Draw(const Texture* texture) const {
	vao.Bind();

	if (texture) {
		texture->Bind(0);  // Bind texture to slot 0
	}

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

	if (texture) {
		texture->Unbind();
	}

	vao.Unbind();
}

void Mesh::SetupInstanceBuffer(const std::vector<InstanceData>& instanceData) {
	if (instanceData.empty()) return;

	if (!instanceBufferInitialized) {
		glGenBuffers(1, &instanceVBO);
		instanceBufferInitialized = true;
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

	vao.Bind();

	// modelMatrix starts at offset offsetof(InstanceData, modelMatrix)
	const std::size_t matOffset = offsetof(InstanceData, modelMatrix);
	const std::size_t vec4Size = sizeof(glm::vec4);
	const GLsizei stride = static_cast<GLsizei>(sizeof(InstanceData));

	for (int i = 0; i < 4; ++i) {
		const void* ptr = reinterpret_cast<const void*>(matOffset + i * vec4Size);
		glEnableVertexAttribArray(2 + i);
		glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, stride, ptr);
		glVertexAttribDivisor(2 + i, 1);
	}

	// UV offset/scale
	const void* uvPtr = reinterpret_cast<const void*>(offsetof(InstanceData, uvOffsetScale));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, uvPtr);
	glVertexAttribDivisor(6, 1);

	vao.Unbind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void Mesh::DrawInstanced(Texture* texture, size_t instanceCount) {
	if (instanceCount == 0) return;

	// Bind texture if provided
	if (texture) {
		texture->Bind(0);
	}

	// Bind VAO and draw instanced
	vao.Bind();
	glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, static_cast<GLsizei>(instanceCount));
	glBindVertexArray(0);
}
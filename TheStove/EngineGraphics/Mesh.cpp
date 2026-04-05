/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Mesh.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Binds vertex attributes for supported layouts and issues glDrawArrays calls.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/Mesh.hpp"

/**
 * @brief Uploads vertex data and configures the mesh's vertex layout.
 * @param vertices Pointer to interleaved vertex data.
 * @param vertexCount Number of vertices contained in the buffer.
 * @param vertexSize Size in bytes of a single vertex.
 * @param layout Attribute layout to bind inside the VAO.
 */
Mesh::Mesh(const float* vertices, GLsizei vertexCount, GLsizei vertexSize, VertexLayout layout)
	: vao(), vbo(vertices, vertexCount* vertexSize), vertexCount(static_cast<GLsizei>(vertexCount)) {

	// Bind only the attributes required by the chosen vertex format.
	switch (layout) {
	case POSITION_COLOR:
		// Route position and color attributes to the shader's expected slots.
		vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);
		vao.AddBuffer(vbo, 1, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));
		break;

	case POSITION_TEXTURE:
		// Route position and UV attributes to the shader's expected slots.
		vao.AddBuffer(vbo, 0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);
		vao.AddBuffer(vbo, 1, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));
		break;
	}
}

/**
 * @brief Draws the mesh with the currently bound shader state.
 */
void Mesh::Draw() const {
	// Activate the mesh VAO before issuing any draw work.
	vao.Bind();

	// Surface any stale GL state issues before the actual draw call.
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[Mesh] OpenGL error after VAO bind: " << error);
	}

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

	// Report draw-call failures immediately while the error source is still obvious.
	error = glGetError();
	if (error != GL_NO_ERROR) {
		TS_LOG_ERROR("[Mesh] OpenGL error after glDrawArrays: " << error);
	}

	// Leave GL state in a neutral VAO state for subsequent callers.
	vao.Unbind();
}

/**
 * @brief Draws the mesh while optionally binding a texture to slot 0.
 * @param texture Texture to bind before drawing, or `nullptr` to draw untextured.
 */
void Mesh::Draw(const Texture* texture) const {
	// Bind the vertex array first so the active shader sees the correct geometry layout.
	vao.Bind();

	if (texture) {
		// Bind the provided texture on the default sprite texture slot.
		texture->Bind(0);
	}

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);

	if (texture) {
		// Release the temporary texture binding once the draw is complete.
		texture->Unbind();
	}

	// Restore a neutral VAO binding for the next draw path.
	vao.Unbind();
}

/**
 * @brief Uploads per-instance data and configures instanced attributes on the VAO.
 * @param instanceData CPU-side instance payload to upload.
 */
void Mesh::SetupInstanceBuffer(const std::vector<InstanceData>& instanceData) {
	// Skip the upload when there is no instance payload to describe.
	if (instanceData.empty()) {
		return;
	}

	if (!instanceBufferInitialized) {
		// Allocate the shared instance VBO only once, then reuse it for later updates.
		glGenBuffers(1, &instanceVBO);
		instanceBufferInitialized = true;
	}

	// Stream the latest per-instance payload into the instance buffer.
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(InstanceData), instanceData.data(), GL_DYNAMIC_DRAW);

	// Configure the instance attributes on the mesh VAO so each draw can consume them.
	vao.Bind();

	// Split the matrix into four vec4 attributes because OpenGL attributes are vec4-sized.
	const std::size_t matOffset = offsetof(InstanceData, modelMatrix);
	const std::size_t vec4Size = sizeof(glm::vec4);
	const GLsizei stride = static_cast<GLsizei>(sizeof(InstanceData));

	for (int i = 0; i < 4; ++i) {
		const void* ptr = reinterpret_cast<const void*>(matOffset + i * vec4Size);
		glEnableVertexAttribArray(2 + i);
		glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, stride, ptr);
		glVertexAttribDivisor(2 + i, 1);
	}

	// Publish the animated-UV payload as a separate per-instance attribute.
	const void* uvPtr = reinterpret_cast<const void*>(offsetof(InstanceData, uvOffsetScale));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, uvPtr);
	glVertexAttribDivisor(6, 1);

	// Clean up the temporary bindings after the VAO captures the instance state.
	vao.Unbind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * @brief Draws the mesh multiple times using the configured instance buffer.
 * @param texture Texture to bind before issuing the draw, or `nullptr` for no texture.
 * @param instanceCount Number of instances to draw.
 */
void Mesh::DrawInstanced(Texture* texture, size_t instanceCount) {
	// Avoid issuing a no-op instanced draw when the caller has no instances to render.
	if (instanceCount == 0) {
		return;
	}

	if (texture) {
		// Bind the shared texture once before dispatching the instanced draw.
		texture->Bind(0);
	}

	// Reuse the configured VAO and per-instance attributes for the instanced dispatch.
	vao.Bind();
	glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, static_cast<GLsizei>(instanceCount));
	glBindVertexArray(0);
}

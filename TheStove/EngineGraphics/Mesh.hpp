/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Mesh.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (80%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (20%)

 DESCRIPTION:		Lightweight wrapper for a VAO + VBO with fixed vertex layouts and draw helpers.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "EngineGraphics/Texture.hpp"
#include "EngineGraphics/VertexArray.hpp"
#include "EngineGraphics/VertexBuffer.hpp"

class Texture;

class Mesh {

public:
	enum VertexLayout {
		POSITION_COLOR,    // position (3) + color (3) = 6 floats
		POSITION_TEXTURE   // position (3) + texcoord (2) = 5 floats
	};

	struct InstanceData {
		glm::mat4 modelMatrix;
		glm::vec4 uvOffsetScale; // x,y offset, z,w scale for UV animation frame
		glm::vec4 colorTint; // RGBA tint per instance
	};

	/**
	 * @brief Uploads vertex data and configures the mesh's vertex layout.
	 * @param vertices Pointer to interleaved vertex data.
	 * @param vertexCount Number of vertices contained in the buffer.
	 * @param vertexSize Size in bytes of a single vertex.
	 * @param layout Attribute layout to bind inside the VAO.
	 */
	Mesh(const float* vertices, GLsizei vertexCount, GLsizei vertexSize, VertexLayout layout = POSITION_COLOR);

	/**
	 * @brief Draws the mesh with the currently bound shader state.
	 */
	void Draw() const;

	/**
	 * @brief Draws the mesh while optionally binding a texture to slot 0.
	 * @param texture Texture to bind before drawing, or `nullptr` to draw untextured.
	 */
	void Draw(const Texture* texture) const;

	/**
	 * @brief Uploads per-instance data and configures instanced attributes on the VAO.
	 * @param instanceData CPU-side instance payload to upload.
	 */
	void SetupInstanceBuffer(const std::vector<InstanceData>& instanceData);

	/**
	 * @brief Draws the mesh multiple times using the configured instance buffer.
	 * @param texture Texture to bind before issuing the draw, or `nullptr` for no texture.
	 * @param instanceCount Number of instances to draw.
	 */
	void DrawInstanced(Texture* texture, size_t instanceCount);

private:
	VertexArray vao;
	VertexBuffer vbo;
	GLsizei vertexCount;

	GLuint instanceVBO = 0;  // Stores per-instance transform and UV data.
	bool instanceBufferInitialized = false;
};

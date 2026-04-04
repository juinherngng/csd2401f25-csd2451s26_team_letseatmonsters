/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MeshLoader.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (90%)
 CO-AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Interleaved vertex data, plus vertexCount and vertexSize for Mesh creation.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineGraphics/MeshLoader.hpp"

/**
 * @brief Builds CPU-side vertex data for a colored triangle.
 * @param outVertices Destination vector that receives interleaved vertex data.
 * @param outVertexCount Receives the number of vertices written.
 * @param outVertexSize Receives the size in bytes of a single vertex.
 */
void MeshLoader::LoadSimpleTriangle(std::vector<float>& outVertices, GLsizei& outVertexCount, GLsizei& outVertexSize) {
	// Provide a minimal colored triangle for renderer bring-up and sanity checks.
	float vertices[] = {
		// positions       // colors
		0.0f, 0.5f, 0.0f,  1, 0, 0,
		0.5f, -0.5f, 0.0f, 0, 1, 0,
		-0.5f, -0.5f,0.0f,  0, 0, 1
	};

	// Publish the vertex metadata expected by Mesh construction.
	outVertexCount = 3;
	outVertexSize = 6 * sizeof(float);
	outVertices.assign(vertices, vertices + outVertexCount * 6);
}

/**
 * @brief Builds CPU-side vertex data for a textured sprite quad.
 * @param vertices Destination vector that receives interleaved vertex data.
 * @param vertexCount Receives the number of vertices written.
 * @param vertexSize Receives the size in bytes of a single vertex.
 */
void MeshLoader::LoadSprite(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize) {
	// Emit two triangles that form a centered quad with standard sprite UVs.
	vertices = {
		-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,
		 0.5f, -0.5f, 0.0f,    1.0f, 1.0f,
		 0.5f,  0.5f, 0.0f,    1.0f, 0.0f,

		-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,
		 0.5f,  0.5f, 0.0f,    1.0f, 0.0f,
		-0.5f,  0.5f, 0.0f,    0.0f, 0.0f
	};

	// Report the geometry metadata alongside the generated vertex payload.
	vertexCount = 6;
	vertexSize = 5 * sizeof(float);
}

/**
 * @brief Builds CPU-side vertex data for a full-screen textured quad.
 * @param vertices Destination vector that receives interleaved vertex data.
 * @param vertexCount Receives the number of vertices written.
 * @param vertexSize Receives the size in bytes of a single vertex.
 */
void MeshLoader::LoadFullscreenQuad(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize) {
	// Emit a quad with the same centered geometry used by fullscreen background rendering.
	vertices = {
		-0.5f, -0.5f, 0.0f,      0.0f, 1.0f,
		 0.5f, -0.5f, 0.0f,      1.0f, 1.0f,
		 0.5f,  0.5f, 0.0f,      1.0f, 0.0f,

		-0.5f, -0.5f, 0.0f,      0.0f, 1.0f,
		 0.5f,  0.5f, 0.0f,      1.0f, 0.0f,
		-0.5f,  0.5f, 0.0f,      0.0f, 0.0f
	};

	// Report the geometry metadata alongside the generated vertex payload.
	vertexCount = 6;
	vertexSize = 5 * sizeof(float);
}

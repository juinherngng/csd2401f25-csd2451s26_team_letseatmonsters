/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MeshLoader.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Static mesh builders that fill CPU-side vertex arrays for common shapes.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <vector>

#include "glad/glad.h"

class MeshLoader {
public:
	/**
	 * @brief Builds CPU-side vertex data for a colored triangle.
	 * @param outVertices Destination vector that receives interleaved vertex data.
	 * @param outVertexCount Receives the number of vertices written.
	 * @param outVertexSize Receives the size in bytes of a single vertex.
	 */
	static void LoadSimpleTriangle(std::vector<float>& outVertices, GLsizei& outVertexCount, GLsizei& outVertexSize);

	/**
	 * @brief Builds CPU-side vertex data for a textured sprite quad.
	 * @param vertices Destination vector that receives interleaved vertex data.
	 * @param vertexCount Receives the number of vertices written.
	 * @param vertexSize Receives the size in bytes of a single vertex.
	 */
	static void LoadSprite(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize);

	/**
	 * @brief Builds CPU-side vertex data for a full-screen textured quad.
	 * @param vertices Destination vector that receives interleaved vertex data.
	 * @param vertexCount Receives the number of vertices written.
	 * @param vertexSize Receives the size in bytes of a single vertex.
	 */
	static void LoadFullscreenQuad(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize);
};

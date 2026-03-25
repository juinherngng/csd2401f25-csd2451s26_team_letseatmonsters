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

#include "glad/glad.h"

#include <vector>

class MeshLoader {
public:
	// Load a simple triangle mesh 
	static void LoadSimpleTriangle(std::vector<float>& outVertices, GLsizei& outVertexCount, GLsizei& outVertexSize);
	static void LoadSprite(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize);
	static void LoadFullscreenQuad(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize);
};

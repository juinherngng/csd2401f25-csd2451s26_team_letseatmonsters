/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			MeshLoader.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Interleaved vertex data, plus vertexCount and vertexSize for Mesh creation.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "MeshLoader.hpp"

void MeshLoader::LoadSimpleTriangle(std::vector<float>& outVertices, GLsizei& outVertexCount, GLsizei& outVertexSize) {
	float vertices[] = {
		// positions       // colors
		0.0f, 0.5f, 0.0f,  1, 0, 0,
		0.5f, -0.5f, 0.0f, 0, 1, 0,
		-0.5f, -0.5f,0.0f,  0, 0, 1
	};
	outVertexCount = 3;
	outVertexSize = 6 * sizeof(float); // pos + color
	outVertices.assign(vertices, vertices + outVertexCount * 6);
}

void MeshLoader::LoadSprite(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize) {
	// Sprite quad vertices (position + texture coordinates)
	vertices = {
		// Positions        // Texture Coordinates
		-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,  // Bottom-left
		 0.5f, -0.5f, 0.0f,    1.0f, 1.0f,  // Bottom-right
		 0.5f,  0.5f, 0.0f,    1.0f, 0.0f,  // Top-right

		-0.5f, -0.5f, 0.0f,    0.0f, 1.0f,  // Bottom-left
		 0.5f,  0.5f, 0.0f,    1.0f, 0.0f,  // Top-right
		-0.5f,  0.5f, 0.0f,    0.0f, 0.0f   // Top-left
	};

	vertexCount = 6;  // 6 vertices for 2 triangles
	vertexSize = 5 * sizeof(float);  // 5 floats per vertex (x,y,z,u,v)
}

void MeshLoader::LoadFullscreenQuad(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize) {
	// Unit quad with texture coordinates for background display
	vertices = {
		// Positions              // Texture Coordinates 
		-0.5f, -0.5f, 0.0f,      0.0f, 1.0f,  // Bottom-left 
		 0.5f, -0.5f, 0.0f,      1.0f, 1.0f,  // Bottom-right   
		 0.5f,  0.5f, 0.0f,      1.0f, 0.0f,  // Top-right 

		-0.5f, -0.5f, 0.0f,      0.0f, 1.0f,  // Bottom-left 
		 0.5f,  0.5f, 0.0f,      1.0f, 0.0f,  // Top-right 
		-0.5f,  0.5f, 0.0f,      0.0f, 0.0f   // Top-left 
	};

	vertexCount = 6;
	vertexSize = 5 * sizeof(float);
}

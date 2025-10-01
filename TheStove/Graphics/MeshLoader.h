#pragma once

#include <vector>
#include "glad/glad.h"

class MeshLoader {
public:
    // Load a simple triangle mesh 
    static void LoadSimpleTriangle(std::vector<float>& outVertices, GLsizei& outVertexCount, GLsizei& outVertexSize);
    static void LoadSprite(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize);
    static void LoadFullscreenQuad(std::vector<float>& vertices, GLsizei& vertexCount, GLsizei& vertexSize);
};
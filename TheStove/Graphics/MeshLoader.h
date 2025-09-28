#pragma once

#include <vector>

class MeshLoader {
public:
    // Load a simple triangle mesh 
    static void LoadSimpleTriangle(std::vector<float>& outVertices, size_t& outVertexCount, size_t& outVertexSize);
    static void LoadSprite(std::vector<float>& vertices, size_t& vertexCount, size_t& vertexSize);
    static void LoadFullscreenQuad(std::vector<float>& vertices, size_t& vertexCount, size_t& vertexSize);
};
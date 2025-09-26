#include "MeshLoader.h"

void MeshLoader::LoadSimpleTriangle(std::vector<float>& outVertices, size_t& outVertexCount, size_t& outVertexSize) {
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

void MeshLoader::LoadSprite(std::vector<float>& vertices, size_t& vertexCount, size_t& vertexSize) {
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

void MeshLoader::LoadFullscreenQuad(std::vector<float>& vertices, size_t& vertexCount, size_t& vertexSize) {
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

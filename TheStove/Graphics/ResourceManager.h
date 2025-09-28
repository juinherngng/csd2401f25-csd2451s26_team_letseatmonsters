#pragma once

#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>


class ResourceManager {
public:
    static ResourceManager& Instance() {
        static ResourceManager instance;
        return instance;
    }

    // Shader management
    Shader* LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    Shader* GetShader(const std::string& name);

    // Mesh management
    Mesh* LoadMesh(const std::string& name, const std::vector<float>& vertices, size_t vertexCount, size_t vertexSize, Mesh::VertexLayout layout = Mesh::POSITION_COLOR);
    Mesh* GetMesh(const std::string& name);

    // Texture management
    Texture* LoadTexture(const std::string& name, const std::string& filePath);
    Texture* GetTexture(const std::string& name);

    // Cleanup
    void Clear();

private:
    ResourceManager() = default;
    ~ResourceManager() { Clear(); }

    // Non-copyable singleton
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
};

#include "ResourceManager.h"
#include <iostream>

Shader* ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        std::cout << "Shader '" << name << "' already loaded, returning existing." << std::endl;
        return it->second.get();
    }

    auto shader = std::make_unique<Shader>(vertexPath, fragmentPath);
    Shader* shaderPtr = shader.get();
    shaders[name] = std::move(shader);

    std::cout << "Loaded shader: " << name << std::endl;
    return shaderPtr;
}

Shader* ResourceManager::GetShader(const std::string& name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return it->second.get();
    }

    std::cerr << "Shader '" << name << "' not found!" << std::endl;
    return nullptr;
}

Mesh* ResourceManager::LoadMesh(const std::string& name, const std::vector<float>& vertices, GLsizei vertexCount, GLsizei vertexSize, Mesh::VertexLayout layout) {
    auto it = meshes.find(name);
    if (it != meshes.end()) {
        std::cout << "Mesh '" << name << "' already loaded, returning existing." << std::endl;
        return it->second.get();
    }

    auto mesh = std::make_unique<Mesh>(vertices.data(), vertexCount, vertexSize, layout);
    Mesh* meshPtr = mesh.get();
    meshes[name] = std::move(mesh);

    std::cout << "Loaded mesh: " << name << std::endl;
    return meshPtr;
}

Mesh* ResourceManager::GetMesh(const std::string& name) {
    auto it = meshes.find(name);
    if (it != meshes.end()) {
        return it->second.get();
    }

    std::cerr << "Mesh '" << name << "' not found!" << std::endl;
    return nullptr;
}

Texture* ResourceManager::LoadTexture(const std::string& name, const std::string& filePath) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        std::cout << "Texture '" << name << "' already loaded, returning existing." << std::endl;
        return it->second.get();
    }

    auto texture = std::make_unique<Texture>();
    if (!texture->LoadFromFile(filePath)) {
        return nullptr;
    }

    Texture* texturePtr = texture.get();
    textures[name] = std::move(texture);

    std::cout << "Loaded texture: " << name << std::endl;
    return texturePtr;
}

Texture* ResourceManager::GetTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second.get();
    }

    std::cerr << "Texture '" << name << "' not found!" << std::endl;
    return nullptr;
}

void ResourceManager::Clear() {
    std::cout << "Clearing ResourceManager..." << std::endl;
    shaders.clear();
    meshes.clear();
	textures.clear();
}

/*
----------------------------------------------------------------------------------------------------
FILE NAME:			ResourceManager.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu

DESCRIPTION:		Singleton cache for loading and retrieving Shaders, Textures, and Meshes by name.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "Shader.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// Forward declaration
class AudioManager;

class ResourceManager {
public:
    static ResourceManager& Instance() {
        static ResourceManager instance;
        return instance;
    }

    // AudioManager injection
    void SetAudioManager(AudioManager* audioMgr);

    // Shader management
    Shader* LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    Shader* GetShader(const std::string& name);

    // Mesh management
    Mesh* LoadMesh(const std::string& name, const std::vector<float>& vertices, GLsizei vertexCount, GLsizei vertexSize, Mesh::VertexLayout layout = Mesh::POSITION_COLOR);
    Mesh* GetMesh(const std::string& name);

    // Texture management
    Texture* LoadTexture(const std::string& name, const std::string& filePath);
    Texture* GetTexture(const std::string& name);

    // Audio management (delegates to AudioManager)
    bool LoadAudio(const std::string& name, const std::string& filePath, bool loop = false, bool stream = false);
    bool HasAudio(const std::string& name) const;
    void UnloadAudio(const std::string& name);
    bool GetAudioInfo(const std::string& name, unsigned int& lengthMs, int& channels, int& bits, float& freq) const;

    // Cleanup
    void Clear();

private:
    ResourceManager() : isCleared(false), audioManager(nullptr) {}
    ~ResourceManager() { Clear(); }

    // Non-copyable singleton
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;

    AudioManager* audioManager; // Non-owning pointer to AudioManager
    bool isCleared;

};

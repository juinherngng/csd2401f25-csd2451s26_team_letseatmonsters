/*
----------------------------------------------------------------------------------------------------
FILE NAME:			ResourceManager.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu
CO-AUTHORS:         Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Singleton cache for loading and retrieving Shaders, Textures, and Meshes by name.
					Loads audio files using FMOD and provides access to FMOD::Sound* by name.

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
#include <fmod.hpp>


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
    Mesh* LoadMesh(const std::string& name, const std::vector<float>& vertices, GLsizei vertexCount, GLsizei vertexSize, Mesh::VertexLayout layout = Mesh::POSITION_COLOR);
    Mesh* GetMesh(const std::string& name);

    // Texture management
    Texture* LoadTexture(const std::string& name, const std::string& filePath);
    Texture* GetTexture(const std::string& name);

    // Audio management - juinherng

	// Set FMOD system instance (injected from AudioManager)
    void SetAudioSystem(FMOD::System* sys) { audioSystem = sys; }   // inject after AudioManager initializes
	// Load, get, unload audio
	FMOD::Sound* LoadAudio(std::string const  name, std::string const& filePath, bool loop = false, bool stream = false);
	FMOD::Sound* GetAudio(std::string const& name) const;
	void UnloadAudio(std::string const& name);
	// Check existence and get info
    bool HasAudio(std::string const& name) const;
	bool GetAudioInfo(std::string const& name, unsigned int& lengthMs, int& channels, int& bits, float& freq) const;  

	// end Audio management

    // Cleanup
    void Clear();

private:
    ResourceManager() : isCleared(false) {}
    ~ResourceManager() { Clear(); }

    // Non-copyable singleton
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;

    bool isCleared;

    // Audio - juinherng
	// wrapping FMOD::Sound* in unique_ptr with custom deleter to ensure proper release
    struct FmodSoundDeleter
    {
        void operator()(FMOD::Sound* s) const noexcept
        {
			if (s) s->release();
        }
    };
    using SoundPtr = std::unique_ptr<FMOD::Sound, FmodSoundDeleter>;
	FMOD::System* audioSystem = nullptr;
	std::unordered_map<std::string, FMOD::Sound*> sounds;

	// end Audio
};

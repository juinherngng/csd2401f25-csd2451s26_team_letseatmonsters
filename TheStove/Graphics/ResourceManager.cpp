/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ResourceManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:		Implements lazy-loading, storage maps, and cleanup for shared GPU resources.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <iostream>

#include "../Core/AudioManager.hpp"
#include "../Core/FontSystem.hpp"

#include "ResourceManager.hpp"

void ResourceManager::SetAudioManager(AudioManager* audioMgr) {
	audioManager = audioMgr;
	if (audioManager) {
		std::cout << "AudioManager set in ResourceManager." << std::endl;
	}
}

Shader* ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
	auto it = shaders.find(name);
	if (it != shaders.end()) {
		std::cout << "[ResourceManager] Shader '" << name << "' already loaded, returning cached version." << std::endl;
		return it->second.get();
	}

	std::cout << "[ResourceManager] Loading NEW shader '" << name << "' from:" << std::endl;
	std::cout << "  Vertex: " << vertexPath << std::endl;
	std::cout << "  Fragment: " << fragmentPath << std::endl;

	auto shader = std::make_unique<Shader>(vertexPath, fragmentPath);
	Shader* shaderPtr = shader.get();
	shaders[name] = std::move(shader);

	std::cout << "[ResourceManager] Successfully loaded shader: " << name << std::endl;
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
		//std::cout << "Mesh '" << name << "' already loaded, returning existing." << std::endl;
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
		//std::cout << "Texture '" << name << "' already loaded, returning existing." << std::endl;
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

// Audio management methods - delegate to AudioManager

bool ResourceManager::LoadAudio(const std::string& name, const std::string& filePath, bool loop, bool stream) {
	if (!audioManager) {
		std::cerr << "AudioManager not set in ResourceManager! Cannot load audio." << std::endl;
		return false;
	}

	auto* sound = audioManager->LoadSound(name, filePath, loop, stream);
	return sound != nullptr;
}

bool ResourceManager::HasAudio(const std::string& name) const {
	if (!audioManager) {
		std::cerr << "AudioManager not set in ResourceManager! Cannot check audio." << std::endl;
		return false;
	}

	return audioManager->HasSound(name);
}

void ResourceManager::UnloadAudio(const std::string& name) {
	if (!audioManager) {
		std::cerr << "AudioManager not set in ResourceManager! Cannot unload audio." << std::endl;
		return;
	}

	audioManager->UnloadSound(name);
}

bool ResourceManager::GetAudioInfo(const std::string& name, unsigned int& lengthMs, int& channels, int& bits, float& freq) const {
	if (!audioManager) {
		std::cerr << "AudioManager not set in ResourceManager! Cannot get audio info." << std::endl;
		return false;
	}

	return audioManager->GetSoundInfo(name, lengthMs, channels, bits, freq);
}

// Font management methods

FontSystem::Font* ResourceManager::LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize) {
	return FontSystem::FontManager::Instance().LoadFont(name, fontPath, fontSize);
}

FontSystem::Font* ResourceManager::GetFont(const std::string& name) {
	return FontSystem::FontManager::Instance().GetFont(name);
}

void ResourceManager::Clear() {
	if (!isCleared) {
		std::cout << "Clearing ResourceManager..." << std::endl;
		shaders.clear();
		meshes.clear();
		textures.clear();
		// Note: Audio is managed by AudioManager, so we don't clear it here
		// Note: Fonts are managed by FontManager, so we don't clear them here
		isCleared = true;
	}
}

/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ResourceManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (50%)

 DESCRIPTION:		Implements lazy-loading, storage maps, and cleanup for shared GPU resources.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Core/AudioManager.hpp"
#include "../Core/FontSystem.hpp"

#include "ResourceManager.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unordered_set>

namespace {
	std::string NormalizePath(const std::string& path) {
		if (path.empty()) {
			return path;
		}

		std::error_code ec;
		auto weakPath = std::filesystem::weakly_canonical(std::filesystem::path(path), ec);
		if (!ec) {
			return weakPath.lexically_normal().string();
		}

		return std::filesystem::path(path).lexically_normal().string();
	}
}

// Caches normalized paths to avoid redundant filesystem calls for the same paths
std::string ResourceManager::NormalizePathCached(const std::string& path) {
	if (path.empty()) {
		return path;
	}

	auto cached = normalizedPathCache.find(path);
	if (cached != normalizedPathCache.end()) {
		return cached->second;
	}

	std::string normalized = NormalizePath(path);
	normalizedPathCache[path] = normalized;
	return normalized;
}

// AudioManager injection
void ResourceManager::SetAudioManager(AudioManager* audioMgr) {
	audioManager = audioMgr;
	if (audioManager) {
		std::cout << "AudioManager set in ResourceManager." << std::endl;
	}
}

// Shader management methods
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

// Mesh management methods
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

// Texture management methods
Texture* ResourceManager::LoadTexture(const std::string& name, const std::string& filePath) {
	auto it = textures.find(name);
	if (it != textures.end()) {
		return it->second.get();
	}

	auto aliasIt = textureAliases.find(name);
	if (aliasIt != textureAliases.end()) {
		return aliasIt->second;
	}

	const std::string normalizedPath = NormalizePathCached(filePath);
	auto pathIt = texturePaths.find(normalizedPath);
	if (pathIt != texturePaths.end()) {
		textureAliases[name] = pathIt->second;
		std::cout << "Reusing texture: " << filePath << " as alias '" << name << "'" << std::endl;
		return pathIt->second;
	}

	if (failedTexturePaths.find(normalizedPath) != failedTexturePaths.end()) {
		return nullptr;
	}

	auto texture = std::make_unique<Texture>();
	if (!texture->LoadFromFile(filePath)) {
		failedTexturePaths.insert(normalizedPath);
		return nullptr;
	}

	Texture* texturePtr = texture.get();
	textures[name] = std::move(texture);
	texturePaths[normalizedPath] = texturePtr;
	failedTexturePaths.erase(normalizedPath);

#ifndef NDEBUG
	std::cout << "Loaded texture: " << name << std::endl;
#endif
	return texturePtr;
}
Texture* ResourceManager::GetTexture(const std::string& name) {
	auto it = textures.find(name);
	if (it != textures.end()) {
		return it->second.get();
	}

	auto aliasIt = textureAliases.find(name);
	if (aliasIt != textureAliases.end()) {
		return aliasIt->second;
	}

	std::cerr << "Texture '" << name << "' not found!" << std::endl;
	return nullptr;
}
void ResourceManager::PreloadTextures(const std::vector<std::string>& filePaths) {
	if (filePaths.empty()) {
		return;
	}

	const auto preloadStart = std::chrono::steady_clock::now();
	double uploadMs = 0.0;
	size_t loadedCount = 0;
	size_t failedCount = 0;

	struct PendingPath {
		std::string path;
		std::string normalizedPath;
	};

	std::vector<PendingPath> uniquePaths;
	uniquePaths.reserve(filePaths.size());
	std::unordered_set<std::string> seen;

	for (const auto& filePath : filePaths) {
		if (filePath.empty()) {
			continue;
		}

		const std::string normalizedPath = NormalizePathCached(filePath);
		if (texturePaths.find(normalizedPath) != texturePaths.end()) {
			continue;
		}

		if (failedTexturePaths.find(normalizedPath) != failedTexturePaths.end()) {
			continue;
		}

		if (seen.insert(normalizedPath).second) {
			uniquePaths.push_back(PendingPath{ filePath, normalizedPath });
		}
	}

	if (uniquePaths.empty()) {
		return;
	}

	struct DecodedTexture {
		std::string path;
		std::string normalizedPath;
		std::vector<unsigned char> data;
		int width = 0;
		int height = 0;
		int channels = 0;
		bool ok = false;
	};

	auto decodeTask = [](PendingPath pendingPath) {
		DecodedTexture decoded;
		decoded.path = std::move(pendingPath.path);
		decoded.normalizedPath = std::move(pendingPath.normalizedPath);
		decoded.ok = Texture::DecodeFile(decoded.path, decoded.data, decoded.width, decoded.height, decoded.channels);
		return decoded;
		};

	const unsigned int hw = std::max(1u, std::thread::hardware_concurrency());
	const size_t maxWorkers = std::max<size_t>(2, hw - 1);
	std::vector<std::future<DecodedTexture>> futures;
	futures.reserve(uniquePaths.size());
	size_t nextFutureToConsume = 0;

	auto consumeDecodedTexture = [&](DecodedTexture&& decoded) {
		if (!decoded.ok) {
			failedTexturePaths.insert(decoded.normalizedPath);
			++failedCount;
			return;
		}

		auto texture = std::make_unique<Texture>();
		const auto uploadStart = std::chrono::steady_clock::now();
		if (!texture->LoadFromMemory(decoded.data.data(), decoded.width, decoded.height, decoded.channels)) {
			failedTexturePaths.insert(decoded.normalizedPath);
			++failedCount;
			return;
		}

		uploadMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - uploadStart).count();
		++loadedCount;

		Texture* texturePtr = texture.get();
		const std::string cacheKey = "preload_" + decoded.path;
		textures[cacheKey] = std::move(texture);
		texturePaths[decoded.normalizedPath] = texturePtr;
		failedTexturePaths.erase(decoded.normalizedPath);
		};

	for (auto& pendingPath : uniquePaths) {
		futures.emplace_back(std::async(std::launch::async, decodeTask, std::move(pendingPath)));
		if (futures.size() - nextFutureToConsume >= maxWorkers) {
			consumeDecodedTexture(futures[nextFutureToConsume++].get());
		}
	}

	while (nextFutureToConsume < futures.size()) {
		consumeDecodedTexture(futures[nextFutureToConsume++].get());
	}

#ifndef NDEBUG
	const double totalMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - preloadStart).count();
	std::cout << "[ResourceManager] Preloaded textures: " << loadedCount << "/" << uniquePaths.size()
		<< " (failed: " << failedCount << ", total: " << totalMs << " ms, upload: " << uploadMs << " ms)" << std::endl;
#endif
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

// Cleanup method
void ResourceManager::Clear() {
	if (!isCleared) {
		std::cout << "Clearing ResourceManager..." << std::endl;
		shaders.clear();
		meshes.clear();
		textureAliases.clear();
		texturePaths.clear();
		normalizedPathCache.clear();
		failedTexturePaths.clear();
		textures.clear();
		// Note: Audio is managed by AudioManager, so we don't clear it here
		// Note: Fonts are managed by FontManager, so we don't clear them here
		isCleared = true;
	}
}

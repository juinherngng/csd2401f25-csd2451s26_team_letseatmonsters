/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ResourceManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (20%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (20%)
					Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		Implements lazy-loading, storage maps, and cleanup for shared GPU resources.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Core/AudioManager.hpp"
#include "../Core/FontSystem.hpp"
#include "../Core/Logger.hpp"

#include "ResourceManager.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <thread>
#include <unordered_set>

namespace {

	/**
	 * @brief Normalizes path.
	 * @param path Path to process.
	 * @return Result produced by this operation.
	 */
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

/**
 * @brief Normalizes path cached.
 * @param path Path to process.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Sets audio manager.
 * @param audioMgr Parameter for audio mgr.
 * @return Result produced by this operation.
 */
void ResourceManager::SetAudioManager(AudioManager* audioMgr) {
	audioManager = audioMgr;
	if (audioManager) {
		TS_LOG_INFO("[ResourceManager] AudioManager registered.");
	}
}

/**
 * @brief Loads shader.
 * @param name Parameter for name.
 * @param vertexPath Parameter for vertex path.
 * @param fragmentPath Parameter for fragment path.
 * @return Result produced by this operation.
 */
Shader* ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
	auto it = shaders.find(name);
	if (it != shaders.end()) {
		TS_LOG_DEBUG("[ResourceManager] Shader '" << name << "' already loaded, returning cached version.");
		return it->second.get();
	}

	TS_LOG_INFO("[ResourceManager] Loading shader '" << name << "' from vertex='" << vertexPath
		<< "', fragment='" << fragmentPath << "'.");

	auto shader = std::make_unique<Shader>(vertexPath, fragmentPath);
	Shader* shaderPtr = shader.get();
	shaders[name] = std::move(shader);

	TS_LOG_INFO("[ResourceManager] Successfully loaded shader: " << name);
	return shaderPtr;
}

/**
 * @brief Returns shader.
 * @param name Parameter for name.
 * @return Requested value.
 */
Shader* ResourceManager::GetShader(const std::string& name) {
	auto it = shaders.find(name);
	if (it != shaders.end()) {
		return it->second.get();
	}

	TS_LOG_ERROR("[ResourceManager] Shader '" << name << "' not found!");
	return nullptr;
}

/**
 * @brief Loads mesh.
 * @param name Parameter for name.
 * @param vertices Parameter for vertices.
 * @param vertexCount Parameter for vertex count.
 * @param vertexSize Parameter for vertex size.
 * @param layout Parameter for layout.
 * @return Result produced by this operation.
 */
Mesh* ResourceManager::LoadMesh(const std::string& name, const std::vector<float>& vertices, GLsizei vertexCount, GLsizei vertexSize, Mesh::VertexLayout layout) {
	auto it = meshes.find(name);
	if (it != meshes.end()) {
		//std::cout << "Mesh '" << name << "' already loaded, returning existing." << std::endl;
		return it->second.get();
	}

	auto mesh = std::make_unique<Mesh>(vertices.data(), vertexCount, vertexSize, layout);
	Mesh* meshPtr = mesh.get();
	meshes[name] = std::move(mesh);

	TS_LOG_DEBUG("[ResourceManager] Loaded mesh: " << name);
	return meshPtr;
}

/**
 * @brief Returns mesh.
 * @param name Parameter for name.
 * @return Requested value.
 */
Mesh* ResourceManager::GetMesh(const std::string& name) {
	auto it = meshes.find(name);
	if (it != meshes.end()) {
		return it->second.get();
	}

	TS_LOG_ERROR("[ResourceManager] Mesh '" << name << "' not found!");
	return nullptr;
}

/**
 * @brief Loads texture.
 * @param name Parameter for name.
 * @param filePath Path to the target file.
 * @return Result produced by this operation.
 */
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
		TS_LOG_DEBUG("[ResourceManager] Reusing texture '" << filePath << "' as alias '" << name << "'.");
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
	TS_LOG_DEBUG("[ResourceManager] Loaded texture: " << name);
#endif
	return texturePtr;
}

/**
 * @brief Returns texture.
 * @param name Parameter for name.
 * @return Requested value.
 */
Texture* ResourceManager::GetTexture(const std::string& name) {
	auto it = textures.find(name);
	if (it != textures.end()) {
		return it->second.get();
	}

	auto aliasIt = textureAliases.find(name);
	if (aliasIt != textureAliases.end()) {
		return aliasIt->second;
	}

	TS_LOG_ERROR("[ResourceManager] Texture '" << name << "' not found!");
	return nullptr;
}

/**
 * @brief Performs preload textures.
 * @param filePaths Parameter for file paths.
 * @return Result produced by this operation.
 */
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
	TS_LOG_INFO("[ResourceManager] Preloaded textures: " << loadedCount << "/" << uniquePaths.size()
		<< " (failed: " << failedCount << ", total: " << totalMs << " ms, upload: " << uploadMs << " ms)");
#endif
}

/**
 * @brief Loads audio.
 * @param name Parameter for name.
 * @param filePath Path to the target file.
 * @param loop Parameter for loop.
 * @param stream Parameter for stream.
 * @return Result produced by this operation.
 */
bool ResourceManager::LoadAudio(const std::string& name, const std::string& filePath, bool loop, bool stream) {
	if (!audioManager) {
		TS_LOG_ERROR("[ResourceManager] AudioManager not set; cannot load audio.");
		return false;
	}

	auto* sound = audioManager->LoadSound(name, filePath, loop, stream);
	return sound != nullptr;
}

/**
 * @brief Loads audio3 d.
 * @param name Parameter for name.
 * @param filePath Path to the target file.
 * @param loop Parameter for loop.
 * @param stream Parameter for stream.
 * @return Result produced by this operation.
 */
bool ResourceManager::LoadAudio3D(const std::string& name, const std::string& filePath, bool loop, bool stream) {
	if (!audioManager) {
		TS_LOG_ERROR("[ResourceManager] AudioManager not set; cannot load 3D audio.");
		return false;
	}

	auto* sound = audioManager->LoadSound3D(name, filePath, loop, stream);
	return sound != nullptr;
}

/**
 * @brief Returns whether audio.
 * @param name Parameter for name.
 * @return True when the operation succeeds or the condition is met.
 */
bool ResourceManager::HasAudio(const std::string& name) const {
	if (!audioManager) {
		TS_LOG_ERROR("[ResourceManager] AudioManager not set; cannot check audio.");
		return false;
	}

	return audioManager->HasSound(name);
}

/**
 * @brief Performs unload audio.
 * @param name Parameter for name.
 * @return Result produced by this operation.
 */
void ResourceManager::UnloadAudio(const std::string& name) {
	if (!audioManager) {
		TS_LOG_ERROR("[ResourceManager] AudioManager not set; cannot unload audio.");
		return;
	}

	audioManager->UnloadSound(name);
}

/**
 * @brief Returns audio info.
 * @param name Parameter for name.
 * @param lengthMs Parameter for length ms.
 * @param channels Parameter for channels.
 * @param bits Parameter for bits.
 * @param freq Parameter for freq.
 * @return Requested value.
 */
bool ResourceManager::GetAudioInfo(const std::string& name, unsigned int& lengthMs, int& channels, int& bits, float& freq) const {
	if (!audioManager) {
		TS_LOG_ERROR("[ResourceManager] AudioManager not set; cannot get audio info.");
		return false;
	}

	return audioManager->GetSoundInfo(name, lengthMs, channels, bits, freq);
}

/**
 * @brief Loads font.
 * @param name Parameter for name.
 * @param fontPath Parameter for font path.
 * @param fontSize Parameter for font size.
 * @return Result produced by this operation.
 */
FontSystem::Font* ResourceManager::LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize) {
	return FontSystem::FontManager::Instance().LoadFont(name, fontPath, fontSize);
}

/**
 * @brief Returns font.
 * @param name Parameter for name.
 * @return Requested value.
 */
FontSystem::Font* ResourceManager::GetFont(const std::string& name) {
	return FontSystem::FontManager::Instance().GetFont(name);
}

/**
 * @brief Clears this object.
 * @return Result produced by this operation.
 */
void ResourceManager::Clear() {
	if (!isCleared) {
		TS_LOG_INFO("[ResourceManager] Clearing cached resources.");
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


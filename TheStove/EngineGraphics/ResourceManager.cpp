/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ResourceManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (20%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Yat Chun Wee, y.chunwee@digipen.edu		(70%)

 DESCRIPTION:		Implements lazy-loading, storage maps, and cleanup for shared GPU resources.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <thread>
#include <unordered_set>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/FontSystem.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/ResourceManager.hpp"

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

	std::string CompactAssetLabel(const std::string& value) {
		if (value.empty()) {
			return value;
		}

		const size_t slashPos = value.find_last_of("/\\");
		if (slashPos != std::string::npos && slashPos + 1 < value.size()) {
			return value.substr(slashPos + 1);
		}

		return value;
	}

	std::string ToLowerAscii(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	Texture::SamplingMode DetermineTextureSamplingMode(const std::string& path) {
		const std::string lower = ToLowerAscii(path);
		const bool shouldSmooth =
			lower.find("\\backgrounds\\") != std::string::npos ||
			lower.find("/backgrounds/") != std::string::npos ||
			lower.find("\\ui\\") != std::string::npos ||
			lower.find("/ui/") != std::string::npos ||
			lower.find("\\credits\\") != std::string::npos ||
			lower.find("/credits/") != std::string::npos ||
			lower.find("\\cutscenes\\") != std::string::npos ||
			lower.find("/cutscenes/") != std::string::npos;

		return shouldSmooth ? Texture::SamplingMode::Smooth : Texture::SamplingMode::PixelArt;
	}

	std::string BuildTextureVariantKey(const std::string& normalizedPath, Texture::SamplingMode samplingMode) {
		return normalizedPath + "|" + (samplingMode == Texture::SamplingMode::Smooth ? "smooth" : "pixel");
	}
}

/**
 * @brief Destroys the `ResourceManager` instance and releases owned resources.
 */
ResourceManager::~ResourceManager() {
	// Reuse the normal cache teardown path so shutdown behavior stays centralized.
	Clear();
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

	// Reuse previously normalized values to avoid repeated filesystem work on hot paths.
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
	// Store the non-owning bridge to the audio system for shared load/unload calls.
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
	// Return the cached shader immediately when the logical name is already registered.
	auto it = shaders.find(name);
	if (it != shaders.end()) {
		TS_LOG_DEBUG("[ResourceManager] Shader '" << name << "' already loaded, returning cached version.");
		return it->second.get();
	}

	TS_LOG_INFO("[ResourceManager] Loading shader '" << name << "' from vertex='" << vertexPath
		<< "', fragment='" << fragmentPath << "'.");

	auto shader = std::make_unique<Shader>(vertexPath, fragmentPath);
	Shader* shaderPtr = shader.get();
	// Promote the newly created shader into the cache before returning the raw pointer.
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
	// Look up shaders by their logical cache name rather than path.
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
		return it->second.get();
	}

	auto mesh = std::make_unique<Mesh>(vertices.data(), vertexCount, vertexSize, layout);
	Mesh* meshPtr = mesh.get();
	// Cache the uploaded mesh once so subsequent calls can share the GPU buffers.
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
	// Resolve meshes by logical name from the shared cache.
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
	// Prefer an existing logical texture binding before doing any path-based work.
	auto it = textures.find(name);
	if (it != textures.end()) {
		return it->second.get();
	}

	auto aliasIt = textureAliases.find(name);
	if (aliasIt != textureAliases.end()) {
		return aliasIt->second;
	}

	// Normalize paths so duplicate relative spellings still share the same GPU texture.
	const std::string normalizedPath = NormalizePathCached(filePath);
	const Texture::SamplingMode samplingMode = DetermineTextureSamplingMode(normalizedPath);
	const std::string variantKey = BuildTextureVariantKey(normalizedPath, samplingMode);
	auto pathIt = texturePaths.find(variantKey);
	if (pathIt != texturePaths.end()) {
		// Record an alias from the requested logical name to the already loaded texture instance.
		textureAliases[name] = pathIt->second;
		TS_LOG_DEBUG("[ResourceManager] Reusing texture '" << CompactAssetLabel(filePath)
			<< "' as '" << CompactAssetLabel(name) << "'.");
		return pathIt->second;
	}

	if (failedTexturePaths.find(variantKey) != failedTexturePaths.end()) {
		// Skip repeated disk attempts for textures that already failed earlier in the run.
		return nullptr;
	}

	auto texture = std::make_unique<Texture>();
	if (!texture->LoadFromFile(filePath, samplingMode)) {
		failedTexturePaths.insert(variantKey);
		return nullptr;
	}

	Texture* texturePtr = texture.get();
	// Store both the logical name and normalized file path so later aliases can share this texture.
	textures[name] = std::move(texture);
	texturePaths[variantKey] = texturePtr;
	failedTexturePaths.erase(variantKey);

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
	// Check both canonical textures and logical aliases before reporting a miss.
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

	// Measure the preload pass so we can track how much time decode and upload take.
	const auto preloadStart = std::chrono::steady_clock::now();
	double uploadMs = 0.0;
	size_t loadedCount = 0;
	size_t failedCount = 0;

	struct PendingPath {
		std::string path;
		std::string normalizedPath;
		std::string variantKey;
		Texture::SamplingMode samplingMode = Texture::SamplingMode::PixelArt;
	};

	std::vector<PendingPath> uniquePaths;
	uniquePaths.reserve(filePaths.size());
	std::unordered_set<std::string> seen;

	for (const auto& filePath : filePaths) {
		// Skip empty, already-loaded, and previously failed paths before scheduling decode work.
		if (filePath.empty()) {
			continue;
		}

		const std::string normalizedPath = NormalizePathCached(filePath);
		const Texture::SamplingMode samplingMode = DetermineTextureSamplingMode(normalizedPath);
		const std::string variantKey = BuildTextureVariantKey(normalizedPath, samplingMode);
		if (texturePaths.find(variantKey) != texturePaths.end()) {
			continue;
		}

		if (failedTexturePaths.find(variantKey) != failedTexturePaths.end()) {
			continue;
		}

		if (seen.insert(variantKey).second) {
			uniquePaths.push_back(PendingPath{ filePath, normalizedPath, variantKey, samplingMode });
		}
	}

	if (uniquePaths.empty()) {
		return;
	}

	struct DecodedTexture {
		std::string path;
		std::string normalizedPath;
		std::string variantKey;
		std::vector<unsigned char> data;
		int width = 0;
		int height = 0;
		int channels = 0;
		bool ok = false;
		Texture::SamplingMode samplingMode = Texture::SamplingMode::PixelArt;
	};

	auto decodeTask = [](PendingPath pendingPath) {
		DecodedTexture decoded;
		decoded.path = std::move(pendingPath.path);
		decoded.normalizedPath = std::move(pendingPath.normalizedPath);
		decoded.variantKey = std::move(pendingPath.variantKey);
		decoded.samplingMode = pendingPath.samplingMode;
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
			failedTexturePaths.insert(decoded.variantKey);
			++failedCount;
			return;
		}

		// Upload decoded image data on the main thread so the OpenGL calls remain valid.
		auto texture = std::make_unique<Texture>();
		const auto uploadStart = std::chrono::steady_clock::now();
		if (!texture->LoadFromMemory(decoded.data.data(), decoded.width, decoded.height, decoded.channels, decoded.samplingMode)) {
			failedTexturePaths.insert(decoded.variantKey);
			++failedCount;
			return;
		}

		uploadMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - uploadStart).count();
		++loadedCount;

		Texture* texturePtr = texture.get();
		const std::string cacheKey = "preload_" + decoded.path;
		// Cache the preload result under a synthetic key and the normalized disk path.
		textures[cacheKey] = std::move(texture);
		texturePaths[decoded.variantKey] = texturePtr;
		failedTexturePaths.erase(decoded.variantKey);
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

	// Delegate actual sound creation to the audio system and return success as a simple boolean.
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

	// Delegate 3D sound creation to the audio system once the bridge is available.
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

	// Ask the audio manager directly because it owns the actual sound cache.
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

	// Forward the unload request to the central audio manager.
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

	// Forward metadata queries so this wrapper stays thin over the audio subsystem.
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
	// Fonts are owned by FontManager, so ResourceManager simply forwards the request.
	return FontSystem::FontManager::Instance().LoadFont(name, fontPath, fontSize);
}

/**
 * @brief Returns font.
 * @param name Parameter for name.
 * @return Requested value.
 */
FontSystem::Font* ResourceManager::GetFont(const std::string& name) {
	// Read font handles directly from the shared font manager cache.
	return FontSystem::FontManager::Instance().GetFont(name);
}

/**
 * @brief Clears this object.
 * @return Result produced by this operation.
 */
void ResourceManager::Clear() {
	if (!isCleared) {
		TS_LOG_INFO("[ResourceManager] Clearing cached resources.");
		// Release GPU-side caches first, then clear path/alias bookkeeping maps.
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

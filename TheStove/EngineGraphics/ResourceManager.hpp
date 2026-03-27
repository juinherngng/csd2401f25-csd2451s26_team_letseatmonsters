/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			ResourceManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (30%)
					Yat Chun Wee, y.chunwee@digipen.edu		(30%)

 DESCRIPTION:		Singleton cache for loading and retrieving Shaders, Textures, and Meshes by name.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "EngineGraphics/Mesh.hpp"
#include "EngineGraphics/Shader.hpp"
#include "EngineGraphics/Texture.hpp"

// Forward declarations
class AudioManager;
namespace FontSystem {
	class Font;
}

class ResourceManager {
public:

	/**
	 * @brief Performs instance.
	 * @return Result produced by this operation.
	 */
	static ResourceManager& Instance() {
		static ResourceManager instance;
		return instance;
	}

	/**
	 * @brief Sets audio manager.
	 * @param audioMgr Parameter for audio mgr.
	 */
	void SetAudioManager(AudioManager* audioMgr);

	/**
	 * @brief Loads shader.
	 * @param name Parameter for name.
	 * @param vertexPath Parameter for vertex path.
	 * @param fragmentPath Parameter for fragment path.
	 * @return Result produced by this operation.
	 */
	Shader* LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);

	/**
	 * @brief Returns shader.
	 * @param name Parameter for name.
	 * @return Requested value.
	 */
	Shader* GetShader(const std::string& name);

	/**
	 * @brief Loads mesh.
	 * @param name Parameter for name.
	 * @param vertices Parameter for vertices.
	 * @param vertexCount Parameter for vertex count.
	 * @param vertexSize Parameter for vertex size.
	 * @param layout Parameter for layout.
	 * @return Result produced by this operation.
	 */
	Mesh* LoadMesh(const std::string& name, const std::vector<float>& vertices, GLsizei vertexCount, GLsizei vertexSize, Mesh::VertexLayout layout = Mesh::POSITION_COLOR);

	/**
	 * @brief Returns mesh.
	 * @param name Parameter for name.
	 * @return Requested value.
	 */
	Mesh* GetMesh(const std::string& name);

	/**
	 * @brief Loads texture.
	 * @param name Parameter for name.
	 * @param filePath Path to the target file.
	 * @return Result produced by this operation.
	 */
	Texture* LoadTexture(const std::string& name, const std::string& filePath);

	/**
	 * @brief Returns texture.
	 * @param name Parameter for name.
	 * @return Requested value.
	 */
	Texture* GetTexture(const std::string& name);

	/**
	 * @brief Performs preload textures.
	 * @param filePaths Parameter for file paths.
	 */
	void PreloadTextures(const std::vector<std::string>& filePaths);

	/**
	 * @brief Loads font.
	 * @param name Parameter for name.
	 * @param fontPath Parameter for font path.
	 * @param fontSize Parameter for font size.
	 * @return Result produced by this operation.
	 */
	FontSystem::Font* LoadFont(const std::string& name, const std::string& fontPath, unsigned int fontSize);

	/**
	 * @brief Returns font.
	 * @param name Parameter for name.
	 * @return Requested value.
	 */
	FontSystem::Font* GetFont(const std::string& name);

	/**
	 * @brief Loads audio.
	 * @param name Parameter for name.
	 * @param filePath Path to the target file.
	 * @param loop Parameter for loop.
	 * @param stream Parameter for stream.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool LoadAudio(const std::string& name, const std::string& filePath, bool loop = false, bool stream = false);

	/**
	 * @brief Loads audio3 d.
	 * @param name Parameter for name.
	 * @param filePath Path to the target file.
	 * @param loop Parameter for loop.
	 * @param stream Parameter for stream.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool LoadAudio3D(const std::string& name, const std::string& filePath, bool loop = false, bool stream = false);

	/**
	 * @brief Returns whether audio.
	 * @param name Parameter for name.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasAudio(const std::string& name) const;

	/**
	 * @brief Performs unload audio.
	 * @param name Parameter for name.
	 */
	void UnloadAudio(const std::string& name);

	/**
	 * @brief Returns audio info.
	 * @param name Parameter for name.
	 * @param lengthMs Parameter for length ms.
	 * @param channels Parameter for channels.
	 * @param bits Parameter for bits.
	 * @param freq Parameter for freq.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool GetAudioInfo(const std::string& name, unsigned int& lengthMs, int& channels, int& bits, float& freq) const;

	/**
	 * @brief Clears this object.
	 */
	void Clear();

	/**
	 * @brief Normalizes path cached.
	 * @param path Path to process.
	 * @return Result produced by this operation.
	 */
	std::string NormalizePathCached(const std::string& path);

private:

	/**
	 * @brief Constructs a `ResourceManager` instance.
	 */
	ResourceManager() : isCleared(false), audioManager(nullptr) {}

	/**
	 * @brief Destroys the `ResourceManager` instance and releases owned resources.
	 */
	~ResourceManager();

	/**
	 * @brief Constructs a `ResourceManager` instance.
	 */
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	// Resource storage
	std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
	std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
	std::unordered_map<std::string, Texture*> textureAliases;
	std::unordered_map<std::string, Texture*> texturePaths;
	std::unordered_map<std::string, std::string> normalizedPathCache;
	std::unordered_set<std::string> failedTexturePaths;

	AudioManager* audioManager; // Non-owning pointer to AudioManager
	bool isCleared;
};
